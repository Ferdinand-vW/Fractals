#include "network/p2p/Client.h"
#include "boost/bind.hpp"
#include "app/Client.h"
#include "common/utils.h"
#include "network/p2p/Connection.h"
#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/Response.h"
#include "torrent/PieceData.h"
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/system/error_code.hpp>
#include <c++/9/bits/c++config.h>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <string>

Client::Client(std::shared_ptr<Connection> connection
              ,std::shared_ptr<Torrent> torrent
              ,boost::asio::io_context &io)
              : m_io(io)
              , m_timer(boost::asio::deadline_timer(io))
              , m_request_cv(std::make_unique<std::condition_variable>())
              ,m_request_mutex(std::make_unique<std::mutex>())
              ,m_connection(connection)
              ,m_torrent(torrent)
              ,cur_piece(std::make_unique<PieceStatus>(
                                PieceStatus{PieceProgress::Nothing,
                                    PieceData{0,0,std::vector<Block>()}
                                    }
                                )
                ) {
    m_client_id = generate_peerId();
    // Pieces are zero based index
    for(int i = 0; i < torrent->m_mi.info.pieces.size();i++) {
        m_missing_pieces.insert(i);
    }
};

bool Client::has_all_pieces() {
    return m_missing_pieces.size() == 0;
}

bool Client::is_choked_by(PeerId p) {
    return m_peer_status[p].m_peer_choking;
}

bool Client::is_connected_to(PeerId p) {
    return m_connection->is_open();
}

void Client::select_piece(PeerId p) {
    int piece = *m_peer_status[p].m_available_pieces.begin();
    m_peer_status[p].m_available_pieces.erase(piece);
    int piece_size = m_torrent->size_of_piece(piece);

    cur_piece->m_data.m_piece_index = piece;
    cur_piece->m_data.m_length = piece_size;
    cur_piece->m_progress = PieceProgress::Nothing;
}

void Client::received_choke(PeerId p) {
    m_peer_status[p].m_peer_choking = true;
}

void Client::received_unchoke(PeerId p) {
    auto ps = m_peer_status[p];
    //Check condition to ensure we don't cancel timer if
    //client had already been unchoked by peer
    if(ps.m_peer_choking) {
        ps.m_peer_choking = false;
        m_timer.cancel();
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
    // m_request_cv->notify_one();
}

void Client::received_bitfield(PeerId p, Bitfield &bf) {
    int piece_index = 0;
    for(auto b : bf.m_bitfield) {
        if(b) { m_peer_status[p].m_available_pieces.insert(piece_index); }
        piece_index++;
    }

    if(m_peer_status[p].m_available_pieces.size() > 0) {
        m_request_cv->notify_one();
    }
}

void Client::received_request(PeerId p, Request &r) {
    cout << "[Client] received receive request from "+ p.m_ip + ":" + std::to_string(p.m_port) << endl;
    cout << r.m_index << " " << r.m_begin << " " << r.m_length << endl;
    cout << "[Client] does not currently support receive request" << endl;
}

void Client::received_piece(PeerId p, Piece &pc) {
    Block b = Block { pc.m_begin, pc.m_block };
    m_timer.cancel();

    cur_piece->m_data.add_block(b);
    if(cur_piece->m_data.is_complete()) {
        
        cout << "[BitTorrent] received all data for " << pc.pprint() << endl;
        
        PieceData piece = cur_piece->m_data;
        m_torrent->write_data(std::move(piece));

        // update internal state of required pieces
        m_missing_pieces.erase(piece.m_piece_index);
        m_existing_pieces.insert(piece.m_piece_index);

        cur_piece.reset();

        if(has_all_pieces()) { //Only report completed if all pieces have been downloaded
            cur_piece->m_progress = PieceProgress::Completed;
            cout << "[BitTorrent] received all pieces" << endl;
        } else { //otherwise we can continue to request pieces
            cur_piece->m_progress = PieceProgress::Nothing;
        }

    }
    else {
        cout << "[BitTorrent] added block to " << pc.pprint() << endl;

        cur_piece->m_progress = PieceProgress::Downloaded;
    }
}

void doNothing(){}

void Client::received_garbage(PeerId p) {
    
}

void Client::send_handshake(HandShake &&hs) {
    m_connection->write_message(std::make_unique<HandShake>(hs),[](boost_error _err,size_t _size){});
}

void Client::receive_handshake() {
    std::deque<char> deq_buf;
    FutureResponse fr = m_connection->timed_blocking_receive(std::chrono::seconds(5));

    if(fr.m_status == std::future_status::timeout) {
        cout << "[BitTorrent] time out on handshake" << endl;
    }
    else {
        std::cout << fr.m_data->size() << std::endl;
        cout << "[BitTorrent] received handshake" << endl;
    }
}


void Client::await_messages(PeerId p) {
    auto f = [this,p](auto err,auto l,auto &&d) {
        handle_peer_message(p, err, l, std::move(d));
    };
    m_connection->on_receive(f);

    m_connection->read_messages();
}

void Client::write_messages(PeerId p) {
    std::cout << "[Client] Write messages" << std::endl;
    auto ps = m_peer_status[p];

    if(ps.m_peer_choking) {
        send_bitfield(p);
        m_timer.expires_from_now(boost::posix_time::seconds(15));
        m_timer.async_wait(
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
    int num_pieces = m_torrent->m_mi.info.pieces.size() / 20;

    // bitfield must be a multiple of 8
    int mult8 = num_pieces % 8;
    int tail = mult8 == 0 ? 0 : 8 - mult8; 
    for(int i = 0; i < num_pieces + tail ;i++) {
        bf.push_back(0);
    }
    std::cout << "pieces " << m_torrent->m_mi.info.pieces.size() << std::endl;
    std::cout << "bitfield " << bf.size() << std::endl;
    auto msg = Bitfield(bf.size(),bf);
    auto msg_ptr = std::make_unique<Bitfield>(msg);

    m_connection->write_message(
          std::move(msg_ptr)
        , boost::bind(&Client::sent_bitfield,this
                    ,boost::asio::placeholders::error
                    ,boost::asio::placeholders::bytes_transferred));
}

void Client::sent_bitfield(const boost_error &error,size_t size) {
    if(error) {
        std::cout << "[Client] " << error.message() << std::endl;
        m_connection->cancel();
        return;
    }
    std::cout << error.message() << std::endl;
    std::cout << size << std::endl;
    std::cout << ">>> Bitfield" << std::endl;
}

void Client::unchoke_timeout(PeerId p,const boost_error &error) {
    if(error != boost::asio::error::operation_aborted) {
        std::cout << "[Client] unchoke time out" << std::endl; 
        m_connection->cancel();
        m_timer.cancel();
    }
}

void Client::send_piece_requests(PeerId p) {
    auto cb = [&](const boost_error &error,size_t size) {
        sent_piece_request(p, error, size);
    };

    if (cur_piece->m_progress == PieceProgress::Nothing) {
        select_piece(p);

        //calculate size to request
        int request_size = 1 << 14 ; // 16KB, standard request size 
        int remaining = cur_piece->m_data.remaining(); // remaining data of piece
        int piece_length = m_torrent->m_mi.info.piece_length; //size of pieces

        Request request(cur_piece->m_data.m_piece_index
                       ,0
                       ,std::min(remaining,std::min(piece_length,request_size)));
        auto req_ptr = std::make_unique<Request>(request);
        m_connection->write_message(std::move(req_ptr),cb);

    } else if (cur_piece->m_progress == PieceProgress::Downloaded) {
        int request_size = 1 << 14; // 16KB
        int remaining = cur_piece->m_data.remaining();
        int piece_length = m_torrent->m_mi.info.piece_length;

        Request request(cur_piece->m_data.m_piece_index
                       ,cur_piece->m_data.next_block_begin()
                       ,std::min(remaining,std::min(piece_length,request_size)));

        auto req_ptr = std::make_unique<Request>(request);
        m_connection->write_message(std::move(req_ptr),cb);
    } else {
        cout << "[BitTorrent] Completed downloading. Leaving thread.." << endl;
    }
}

void Client::sent_piece_request(PeerId p,const boost_error &error, size_t size) {
    std::cout << "[Client] sent piece request to " << p.m_ip << std::endl;

    cur_piece->m_progress = PieceProgress::Requested;

    m_timer.expires_from_now(boost::posix_time::seconds(5));
    m_timer.async_wait(boost::bind(&Client::piece_response_timeout,this,p));
}

void Client::piece_response_timeout(PeerId p) {
    std::cout << "[Client] Timeout on piece response from " << p.m_ip << std::endl;
    m_timer.cancel();
    m_connection->cancel();
}





void Client::handle_peer_message(PeerId p,boost_error error,int length,std::deque<char> &&deq_buf) {
    if(error) {
        std::cout << "[Client] Fatal error: " + error.message() << std::endl;
        m_connection->cancel();
        return;
    }

    auto m = IMessage::parse_message(length, std::move(deq_buf));
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