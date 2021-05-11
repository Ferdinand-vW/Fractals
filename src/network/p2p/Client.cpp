#include "network/p2p/Client.h"
#include "boost/bind.hpp"
#include "app/Client.h"
#include "common/utils.h"
#include "network/p2p/Connection.h"
#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/Response.h"
#include "torrent/PieceData.h"
#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/post.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/system/error_code.hpp>
#include <c++/9/bits/c++config.h>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <string>

Client::Client(std::shared_ptr<Torrent> torrent
              ,boost::asio::io_context &io
              ,std::function<void(PeerId,PeerChange)> on_change_peers)
              : m_io(io)
              ,m_torrent(torrent)
              ,m_on_change_peers(on_change_peers)
              ,m_piece_lock(std::make_unique<std::mutex>())
              ,m_lg(logger::get()) {
    m_client_id = generate_peerId();
    // Pieces are zero based index
    for(int i = 0; i < torrent->m_mi.info.number_of_pieces(); i++) {
        m_missing_pieces.insert(i);
        BOOST_LOG(m_lg) << "piece " << i << " " << torrent->size_of_piece(i);
    }
};

bool Client::has_all_pieces() {
    return m_existing_pieces.size() == m_torrent->m_mi.info.number_of_pieces();
}

bool Client::is_choked_by(PeerId p) {
    return m_peer_status[p].m_peer_choking;
}

bool Client::is_connected_to(PeerId p) {
    return m_connections.find(p) != m_connections.end() && m_connections[p]->is_open();
}

void Client::connect_to_peer(PeerId p) {
    if(m_connections.find(p) != m_connections.end()) {
        BOOST_LOG(m_lg) << "already connected to " << p.m_ip;
        m_on_change_peers(p,PeerChange::Removed);
        return;
    }

    //Construct the connection object
    auto conn_ptr = std::unique_ptr<Connection>(new Connection(m_io,p));
    //From now on the connection must be fetched by reference
    m_connections.insert({ p, std::move(conn_ptr) });

    //Start connecting to the peer
    auto &conn = m_connections[p];
    conn->connect(boost::bind(&Client::connected,this,p,boost::asio::placeholders::error));
    
    //set up timeout of 5 seconds
    auto &timer = conn->get_timer();
    timer.expires_from_now(boost::posix_time::seconds(5));
    timer.async_wait(boost::bind(&Client::connect_timeout,this,p,boost::asio::placeholders::error));
    
}

void Client::connected(PeerId p,const boost_error &error) {
    if(!error) {
        BOOST_LOG(m_lg) << "[Client] connected to " << p.m_ip;
        m_connections[p]->get_timer().cancel();
        //report that we've successfully connected to a peer
        m_on_change_peers(p,PeerChange::Added);
    }
}

void Client::connect_timeout(PeerId p, const boost_error &error) {
    if (error != boost::asio::error::operation_aborted) {
        BOOST_LOG(m_lg) << "[Client] connection timeout with " << p.m_ip;
        drop_connection(p);
    }
}

void Client::drop_connection(PeerId p) {
    if (m_connections.find(p) == m_connections.end()) {
        BOOST_LOG(m_lg) << "already dropped peer " << p.m_ip;
        return ;
    } else {
        BOOST_LOG(m_lg) << "drop peer " << p.m_ip;
        for(auto &kvp : m_connections) {
            BOOST_LOG(m_lg) << kvp.first.m_ip;
        }
    }
    m_connections[p]->cancel(); //ensure connection is closed
    m_connections.erase(p); //remove the connection

    //if we were exchanging data with peer then
    //we may need to correct some book keeping
    if(m_progress.find(p) != m_progress.end()) {
        int piece = m_progress[p]->m_data.m_piece_index;
        { //ensure that a piece is not lost when we drop the peer
            std::unique_lock<std::mutex> lock(*m_piece_lock.get());
            if(piece != -1 && m_existing_pieces.find(piece) == m_existing_pieces.end() && m_missing_pieces.find(piece) == m_missing_pieces.end()) {
                m_missing_pieces.insert(piece);
            }
        }
    }
    
    m_progress.erase(p); // remove held piece progress of peer
    m_on_change_peers(p,PeerChange::Removed); //notify observer that we've dropped a peer
}

void Client::select_piece(PeerId p) {
    int piece = -1;
    { //lock to ensure peers don't choose the same piece
        std::unique_lock<std::mutex> lock(*m_piece_lock.get());
        
        auto &available = m_peer_status[p].m_available_pieces;
        std::set<int> interesting_pieces;

        std::set_intersection(available.begin(),available.end()
                            ,m_missing_pieces.begin(),m_missing_pieces.end()
                            ,std::inserter(interesting_pieces,interesting_pieces.begin()));


        //if the peer has no pieces we are interested in then we can drop the peer
        if(interesting_pieces.empty()) {
            BOOST_LOG(m_lg) << "no pieces left";
            drop_connection(p);
            return;
        }

        //assume that peer has at least one piece that we are interested in...
        piece = *interesting_pieces.begin(); //select the first piece
        m_missing_pieces.erase(piece); //make sure we don't download the same piece from other peers
    }

    // m_peer_status[p].m_available_pieces.erase(piece); // too early?
    int piece_size = m_torrent->size_of_piece(piece);
    auto &piece_ptr = m_progress[p];
    piece_ptr->m_data.m_piece_index = piece;
    piece_ptr->m_data.m_length = piece_size;
    piece_ptr->m_progress = PieceProgress::Nothing;
    piece_ptr->m_data.m_blocks.clear();
    BOOST_LOG(m_lg) << "[Client] " + p.m_ip + " selected piece: " << piece_ptr->m_data.m_piece_index;
    BOOST_LOG(m_lg) << "[Client] " + p.m_ip + " selected piece size: " << piece_ptr->m_data.m_length;
}

void Client::add_peer_progress(PeerId p) {
    auto piece_ptr = std::make_unique<PieceStatus>();
    piece_ptr->m_data.m_piece_index = -1;
    piece_ptr->m_data.m_length = -1;
    piece_ptr->m_data.m_blocks.clear(); // empty existing data if present
    piece_ptr->m_progress = PieceProgress::Nothing;
    m_progress.insert({p,std::move(piece_ptr)});
}

void Client::received_choke(PeerId p) {
    m_peer_status[p].m_peer_choking = true;
    //if we get choked then we can just stop the connection
    drop_connection(p);
}

void Client::received_unchoke(PeerId p) {
    auto& ps = m_peer_status[p];
    //Check condition to ensure we don't cancel timer if
    //client had already been unchoked by peer
    if(ps.m_peer_choking) {
        ps.m_peer_choking = false;
        m_connections[p]->get_timer().cancel();
        write_messages(p); // start write message loop
    }
}

void Client::received_interested(PeerId p) {
    m_peer_status[p].m_peer_interested = true;
}

void Client::received_not_interested(PeerId p) {
    m_peer_status[p].m_peer_interested = false;
}

void Client::received_have(PeerId p, Have &h) {
    m_peer_status[p].m_available_pieces.insert(h.m_piece_index);
    send_interested(p); //only sends a message if not interested yet
}

void Client::received_bitfield(PeerId p, Bitfield &bf) {
    int piece_index = 0;
    std::vector<bool> vec_bools = bytes_to_bitfield(bf.m_bitfield.size(),bf.m_bitfield);
    
    for(auto b : vec_bools) {
        if(b) { m_peer_status[p].m_available_pieces.insert(piece_index); }
        piece_index++;
    }

    send_interested(p); //only sends a message if not interested yet
}

void Client::received_request(PeerId p, Request &r) {
    BOOST_LOG(m_lg) << "[Client] received receive request from "+ p.m_ip + ":" + std::to_string(p.m_port);
    BOOST_LOG(m_lg) << r.m_index << " " << r.m_begin << " " << r.m_length;
    BOOST_LOG(m_lg) << "[Client] does not currently support receive request";
}

void Client::received_piece(PeerId p, Piece &pc) {
    BOOST_LOG(m_lg) << "[Client] received piece " << pc.m_index << " from " << p.m_ip;
    Block b = Block { pc.m_begin, pc.m_block };
    m_connections[p]->get_timer().cancel();

    auto &piece_ptr = m_progress[p];

    piece_ptr->m_data.add_block(b);
    if(piece_ptr->m_data.is_complete()) {
        
        BOOST_LOG(m_lg) << "[BitTorrent] received all data from " << p.m_ip << " for " << pc.pprint();
        
        PieceData piece = piece_ptr->m_data;
        m_torrent->write_data(std::move(piece));

        // update internal state of required pieces
        m_missing_pieces.erase(piece.m_piece_index);
        m_existing_pieces.insert(piece.m_piece_index);

        if(has_all_pieces()) { //Only report completed if all pieces have been downloaded
            piece_ptr->m_progress = PieceProgress::Completed;
            BOOST_LOG(m_lg) << "[BitTorrent] received all pieces";
        } else { //otherwise we can continue to request pieces
            piece_ptr->m_progress = PieceProgress::Nothing;
            write_messages(p);
        }

    }
    else {
        BOOST_LOG(m_lg) << "[BitTorrent] added block to " << pc.pprint();

        piece_ptr->m_progress = PieceProgress::Downloaded;
        write_messages(p);
    }
}

void doNothing(){}

void Client::received_garbage(PeerId p) {
    
}

void Client::send_handshake(PeerId p,HandShake &&hs) {
    m_connections[p]->write_message(std::make_unique<HandShake>(hs),[](boost_error _err,size_t _size){});
}

void Client::handle_peer_handshake(PeerId p,const boost_error &error,int length, std::deque<char> &&deq_buf) {
    if(!error) {
        m_connections[p]->get_timer().cancel();
        auto hs = HandShake::from_bytes_repr(length, deq_buf);
        //TODO : check integrity of hs
        BOOST_LOG(m_lg) << hs->pprint() << " <<< " << p.m_ip;

        //indicate that there's data exchange occurring with this peer
        add_peer_progress(p);
        //Successful handshake so now we can start communicating with client
        await_messages(p);
        write_messages(p);
    } else {
        drop_connection(p);
    }
}

void Client::handshake_timeout(PeerId p, const boost_error& error) {
    if(error != boost::asio::error::operation_aborted) {
        BOOST_LOG(m_lg) << "[Client] handshake timeout with " << p.m_ip;
        drop_connection(p);
        return;
    }
}

void Client::await_handshake(PeerId p) {

    auto f = [this,p](auto err,auto l, auto &&d) {
        BOOST_LOG(m_lg) << "handle peer f";
        handle_peer_handshake(p, err, l, std::move(d));
    };

    if(m_connections.find(p) == m_connections.end()) {
        BOOST_LOG(m_lg) << "not here" << p.m_ip;
    }

    auto &timer = m_connections[p]->get_timer();
    timer.expires_from_now(boost::posix_time::seconds(10));
    timer.async_wait(boost::bind(&Client::handshake_timeout,this,p,boost::asio::placeholders::error));

    m_connections[p]->on_handshake(f);
    m_connections[p]->read_handshake();
    
}


void Client::await_messages(PeerId p) {
    auto f = [this,p](auto err,auto l,auto &&d) {
        handle_peer_message(p, err, l, std::move(d));
    };
    m_connections[p]->on_receive(f);
    m_connections[p]->read_messages();
}

void Client::write_messages(PeerId p) {
    auto ps = m_peer_status[p];
    
    if(ps.m_peer_choking) {
        send_bitfield(p);
        auto &timer = m_connections[p]->get_timer();
        timer.expires_from_now(boost::posix_time::millisec(15000));
        timer.async_wait(
            boost::bind(
                &Client::unchoke_timeout,this,p
                ,boost::asio::placeholders::error()
                )
            );
    }
    else {
        send_piece_requests(p);
    }
}

void Client::send_bitfield(PeerId p) {
    std::vector<bool> bf;
    //pieces is a byte string consisting of consecutive 20length SHA1 hashes
    //thus the number of pieces is the length of the byte string divided by 20
    int num_pieces = m_torrent->m_mi.info.number_of_pieces();

    // bitfield must be a multiple of 8
    int mult8 = num_pieces % 8;
    int tail = mult8 == 0 ? 0 : 8 - mult8; 
    for(int i = 0; i < num_pieces + tail ;i++) {
        bf.push_back(0);
    }
    auto msg = Bitfield(bf);
    auto msg_ptr = std::make_unique<Bitfield>(msg);

    BOOST_LOG(m_lg) << p.m_ip << " >>> " << msg.pprint();

    m_connections[p]->write_message(
          std::move(msg_ptr)
        , boost::bind(&Client::sent_bitfield,this,p
                    ,boost::asio::placeholders::error
                    ,boost::asio::placeholders::bytes_transferred));
}

void Client::sent_bitfield(PeerId p,const boost_error &error,size_t size) {
    if(error) {
        BOOST_LOG(m_lg) << "[Client] sending bitfield to " << p.m_ip << " failed with " << error.message();
        m_connections[p]->cancel();
        return;
    }
}

void Client::send_interested(PeerId p) {
    if(!m_peer_status[p].m_am_interested && m_peer_status[p].m_available_pieces.size() > 0) {
        auto msg_ptr = std::make_unique<Interested>(Interested());
        BOOST_LOG(m_lg) << p.m_ip << " >>> " << msg_ptr->pprint();
        m_connections[p]->write_message(std::move(msg_ptr)
            ,boost::bind(&Client::sent_interested,this,p
                    ,boost::asio::placeholders::error
                    ,boost::asio::placeholders::bytes_transferred));
    }
}

void Client::sent_interested(PeerId p,const boost_error &error,size_t size) {
    if(error) {
        BOOST_LOG(m_lg) << "[Client] sending interested to " << p.m_ip << " failed with " << error.message();
        m_connections[p]->cancel();
        return;
    } else {
        m_peer_status[p].m_am_interested = true;
    }
}

void Client::unchoke_timeout(PeerId p,const boost_error &error) {
    if(error != boost::asio::error::operation_aborted) {
        BOOST_LOG(m_lg) << "[Client] unchoke time out " << p.m_ip; 
        drop_connection(p);
    }
}

void Client::send_piece_requests(PeerId p) {
    auto cb = [&](const boost_error &error,size_t size) {
        sent_piece_request(p, error, size);
    };
    if(m_progress.find(p) == m_progress.end()) {
        select_piece(p);
    }

    auto &piece_ptr = m_progress[p];
    if (piece_ptr->m_progress == PieceProgress::Nothing) {
        select_piece(p);
        
        //calculate size to request
        int request_size = 1 << 14 ; // 16KB, standard request size 
        int remaining = piece_ptr->m_data.remaining(); // remaining data of piece
        int piece_length = m_torrent->m_mi.info.piece_length; //size of pieces

        Request request(piece_ptr->m_data.m_piece_index
                       ,0
                       ,std::min(remaining,std::min(piece_length,request_size)));
        BOOST_LOG(m_lg) << p.m_ip + " >>> " + request.pprint();
        auto req_ptr = std::make_unique<Request>(request);
        m_connections[p]->write_message(std::move(req_ptr)
            ,boost::bind(&Client::sent_piece_request,this,p
                       ,boost::asio::placeholders::error
                       ,boost::asio::placeholders::bytes_transferred));

        return;
    } else if (piece_ptr->m_progress == PieceProgress::Downloaded) {
        int request_size = 1 << 14; // 16KB
        int remaining = piece_ptr->m_data.remaining();
        int piece_length = m_torrent->m_mi.info.piece_length;

        Request request(piece_ptr->m_data.m_piece_index
                       ,piece_ptr->m_data.next_block_begin()
                       ,std::min(remaining,std::min(piece_length,request_size)));
        BOOST_LOG(m_lg) << p.m_ip << " >>> " << request.pprint();
        auto req_ptr = std::make_unique<Request>(request);
        m_connections[p]->write_message(std::move(req_ptr)
            ,boost::bind(&Client::sent_piece_request,this,p
                       ,boost::asio::placeholders::error
                       ,boost::asio::placeholders::bytes_transferred));
    } else {
        BOOST_LOG(m_lg) << "[BitTorrent] Completed downloading. Leaving thread..";
    }
}

void Client::sent_piece_request(PeerId p,const boost_error &error, size_t size) {
    if(error) {
        BOOST_LOG(m_lg) << "[Client] sending piece request to " << p.m_ip << " failed with " << error.message();
        return;
    }
    
    if(m_progress.find(p) == m_progress.end()) {
        BOOST_LOG(m_lg) << "will segment fault due to progress " << p.m_ip;
    }
    m_progress[p]->m_progress = PieceProgress::Requested;

    if(m_connections.find(p) == m_connections.end()) {
        BOOST_LOG(m_lg) << "will segment fault due to connections " << p.m_ip;
    }
    auto &timer = m_connections[p]->get_timer();
    timer.expires_from_now(boost::posix_time::seconds(15));
    timer.async_wait(
        boost::bind(
            &Client::piece_response_timeout,this,p
            ,boost::asio::placeholders::error));
}

void Client::piece_response_timeout(PeerId p,const boost_error &error) {
    if(error != boost::asio::error::operation_aborted) {
        BOOST_LOG(m_lg) << "[Client] piece response time out " << p.m_ip; 
        drop_connection(p);
    }
}





void Client::handle_peer_message(PeerId p,const boost_error &error,int length,std::deque<char> &&deq_buf) {
    if(error) {
        BOOST_LOG(m_lg) << "[Client] Fatal error " + p.m_ip + " " + error.message();
        drop_connection(p);
        return;
    }

    if(deq_buf.size() == 0) { //Received a KeepAlive message which we ignore
        return;
    }

    auto m = IMessage::parse_message(p,length, std::move(deq_buf));
    switch(m->get_messageType().value()) {
        case MessageType::MT_Choke: 
            received_choke(p);
            break;
        case MessageType::MT_UnChoke: 
            received_unchoke(p);
            break;
        case MessageType::MT_Interested: 
            received_interested(p);
            break;
        case MessageType::MT_NotInterested: 
            received_not_interested(p);
            break;
        case MessageType::MT_Have: 
            received_have(p,*static_cast<Have*>(m.get()));
            break;
        case MessageType::MT_Bitfield: 
            received_bitfield(p, *static_cast<Bitfield*>(m.get()));
            break;
        case MessageType::MT_Request: 
            received_request(p, *static_cast<Request*>(m.get()));
            break;
        case MessageType::MT_Piece: 
            received_piece(p, *static_cast<Piece*>(m.get()));
            break;
        case MessageType::MT_Cancel: break;
        case MessageType::MT_Port: break;
    }
}