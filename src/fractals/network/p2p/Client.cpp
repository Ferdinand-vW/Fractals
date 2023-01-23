#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

#include <boost/asio/error.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/system/error_code.hpp>

#include "fractals/app/Client.h"
#include "fractals/common/utils.h"
#include "fractals/common/logger.h"
#include "fractals/network/p2p/Client.h"
#include "fractals/network/p2p/Connection.h"
#include "fractals/network/p2p/Message.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/PeerManager.h"
#include "fractals/network/p2p/PeerWork.h"
#include "fractals/persist/data.h"
#include "fractals/torrent/Piece.h"
#include "fractals/torrent/Torrent.h"

using namespace fractals::torrent;
using namespace fractals::persist;
using namespace fractals::network::http;

namespace fractals::network::p2p {

    Client::Client(int max_peers
                ,std::shared_ptr<Torrent> torrent
                ,boost::asio::io_context &io
                ,Storage &storage
                ,std::function<void(PeerId,PeerChange)> on_change_peers)
                : m_peers(PeerManager(max_peers)) 
                ,m_io(io)
                ,m_torrent(torrent)
                ,m_storage(storage)
                ,m_on_change_peers(on_change_peers)
                ,m_piece_lock(std::make_unique<std::mutex>())
                ,m_lg(common::logger::get()) {
        m_client_id = app::generate_peerId();

        m_existing_pieces = m_torrent->get_pieces();
        // Pieces are zero based index
        for(int i = 0; i < torrent->getMetaInfo().info.number_of_pieces(); i++) {
            //piece has not been downloaded yet so we wish to download it
            if(m_existing_pieces.find(i) == m_existing_pieces.end()) {
                m_missing_pieces.insert(i);
            }
        }
    };

    void Client::close_connections() {
        m_peers.disable();
    }

    bool Client::is_enabled() {
        return m_peers.is_enabled();
    }

    bool Client::has_all_pieces() {
        return m_existing_pieces.size() == m_torrent->getMetaInfo().info.number_of_pieces();
    }

    bool Client::is_choked_by(PeerId p) {
        return m_peer_status[p].m_peer_choking;
    }

    bool Client::is_connected_to(PeerId p) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return false; }
        auto conn = mconn.value();

        return m_peers.is_connected_to(p) && conn->is_open();
    }

    int Client::num_connections() {
        return m_peers.num_connections();
    }

    void Client::connect_to_peer(PeerId p) {
        //if provided peer id is a peer we have already established connections with
        //then we should try anew
        if(m_peers.is_connected_to(p)) {
            BOOST_LOG(m_lg) << "already connected to " << p.m_ip;
            m_on_change_peers(p,PeerChange::Removed);
            return;
        }

        //Construct the connection object
        auto conn = std::shared_ptr<Connection>(new Connection(m_io,p));
        //From now on the connection must be fetched by reference
        bool res = m_peers.add_new_connection(p,conn);

        //if res = false then the connection manager is disabled
        //therefore we must now stop, otherwise we can continue.
        //If the ConnectionManager is disabled right after the addition of the above connection
        //then since we use a shared_ptr our resource will not be deleted until it goes out of scope
        //furthermore the async operations will simply error out
        if(!res) {
            return;
        }

        conn->connect(boost::bind(&Client::connected,this,p,boost::asio::placeholders::error));
        
        //set up timeout of 5 seconds
        auto &timer = conn->get_timer();
        timer.async_wait(boost::posix_time::seconds(5)
                        ,boost::bind(&Client::connect_timeout,this,p
                        ,boost::asio::placeholders::error));
        
    }

    void Client::connected(PeerId p,const boost_error &error) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        if(!error) {
            BOOST_LOG(m_lg) << "[Client] connected to " << p.m_ip;
            mconn.value()->get_timer().cancel();
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
        auto mconn = m_peers.lookup(p);
        if (!mconn.has_value()) {
            BOOST_LOG(m_lg) << "already dropped peer " << p.m_ip;
            return ;
        } else {
            BOOST_LOG(m_lg) << "drop peer " << p.m_ip;
        }
        auto conn = mconn.value(); 
        conn->cancel(); //ensure connection is closed
        m_peers.remove_connection(p);

        //if peer had work then reinsert the piece into missing pieces data struct
        auto work = m_peers.lookup_work(p);
        if(work.has_value()) {
            m_missing_pieces.insert(work->get()->m_data.m_piece_index);
            m_peers.finished_work(p);
        }

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
        auto piece_size = torrent::size_of_piece(m_torrent->getMeta(),piece);
        add_peer_work(p,piece,piece_size);

        BOOST_LOG(m_lg) << "[Client] " + p.m_ip + " selected piece: " << piece;
        BOOST_LOG(m_lg) << "[Client] " + p.m_ip + " selected piece size: " << piece_size;
    }

    void Client::add_peer_work(PeerId p,int piece,int64_t piece_size) {
        torrent::Piece pd(piece, piece_size);
        PeerWork pw = { PieceProgress::Nothing, pd };

        auto work_ptr = std::make_shared<PeerWork>(pw);
        m_peers.new_work(p, work_ptr);
    }

    void Client::received_choke(PeerId p) {
        m_peer_status[p].m_peer_choking = true;
        //if we get choked then we can just drop the connection
        drop_connection(p);
    }

    void Client::received_unchoke(PeerId p) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        auto& ps = m_peer_status[p];
        //Check condition to ensure we don't cancel timer if
        //client had already been unchoked by peer
        if(ps.m_peer_choking) {
            ps.m_peer_choking = false;
            mconn.value()->get_timer().cancel();
            write_messages(p); // start write message loop
        }
        BOOST_LOG(m_lg) << "received unchoked: " << ps.m_peer_choking;
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
        // std::vector<bool> vec_bools = common::bytes_to_bitfield(bf.m_bitfield.size(),bf.m_bitfield);
        
        // for(auto b : vec_bools) {
        //     if(b) { m_peer_status[p].m_available_pieces.insert(piece_index); }
        //     piece_index++;
        // }

        send_interested(p); //only sends a message if not interested yet
    }

    //unimplemented since we don't actually support data requests at this time
    void Client::received_request(PeerId p, Request &r) {
        BOOST_LOG(m_lg) << "[Client] received receive request from "+ p.m_ip + ":" + std::to_string(p.m_port);
        BOOST_LOG(m_lg) << r.m_index << " " << r.m_begin << " " << r.m_length;
        BOOST_LOG(m_lg) << "[Client] does not currently support receive request";
    }

    void Client::received_piece(PeerId p, Piece &pc) {
        auto mconn = m_peers.lookup(p);
        auto mwork = m_peers.lookup_work(p);
        //if we can't find the work for this peer then we should assume peer manager has been canceled
        if(!mwork.has_value() || !mconn.has_value()) { return; }
        auto conn = mconn.value();
        auto work = mwork.value();
        
        BOOST_LOG(m_lg) << "[Client] received piece " << pc.m_index << " from " << p.m_ip;
        Block b = Block { pc.m_begin, pc.m_block };
        conn->get_timer().cancel();

        work->m_data.add_block(b);
        if(work->m_data.is_complete()) {
            
            BOOST_LOG(m_lg) << "[Client] received all data from " << p.m_ip << " for " << pc.pprint();
            torrent::Piece piece = work->m_data;
            int pieceIndex = piece.m_piece_index; // retain relevant information
            BOOST_LOG(m_lg) << pieceIndex;
            m_torrent->writePiece(std::move(piece)); // other data is now discarded

            // update internal state of required pieces
            m_missing_pieces.erase(pieceIndex);
            m_existing_pieces.insert(pieceIndex);
            m_peers.finished_work(p);
            save_piece(m_storage,m_torrent->getMeta(),pieceIndex);

            if(has_all_pieces()) { //Only report completed if all pieces have been downloaded
                BOOST_LOG(m_lg) << "[BitTorrent] received all pieces";
            } else { //otherwise we can continue to request pieces
                write_messages(p);
            }

        }
        else {
            BOOST_LOG(m_lg) << "[BitTorrent] added block to " << pc.pprint();

            work->m_progress = PieceProgress::Downloaded;
            write_messages(p);
        }
    }

    void Client::send_handshake(PeerId p,HandShake &&hs) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        mconn.value()->write_message(
                std::make_unique<HandShake>(hs),[](boost_error _err,size_t _size){}
                );
    }

    void Client::handle_peer_handshake(PeerId p,const boost_error &error,int length, std::deque<char> &&deq_buf) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        
        if(!error) {
            mconn.value()->get_timer().cancel();
            auto hs = HandShake::from_bytes_repr(length, deq_buf);
            //TODO : check integrity of hs
            BOOST_LOG(m_lg) << hs->pprint() << " <<< " << p.m_ip;

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
        auto mpeer = m_peers.lookup(p);
        //peer has already been removed
        if(!mpeer.has_value()) { return; }
        auto f = [this,p](auto err,auto l, auto &&d) {
            handle_peer_handshake(p, err, l, std::move(d));
        };

        auto peer = mpeer.value();

        auto &timer = peer->get_timer();
        timer.async_wait(boost::posix_time::seconds(10)
                        ,boost::bind(&Client::handshake_timeout,this,p
                                    ,boost::asio::placeholders::error));

        peer->on_handshake(f);
        peer->read_handshake();
        
    }


    void Client::await_messages(PeerId p) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        auto conn = mconn.value();

        auto f = [this,p](auto err,auto l,auto &&d) {
            handle_peer_message(p, err, l, std::move(d));
        };
        conn->on_receive(f);
        conn->read_messages(); //starts read messages loop for this peer
    }

    void Client::write_messages(PeerId p) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        auto conn = mconn.value();

        auto ps = m_peer_status[p];
        
        if(ps.m_peer_choking) {
            send_bitfield(p);
            auto &timer = conn->get_timer();
            timer.async_wait(boost::posix_time::seconds(15)
                ,boost::bind(
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
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        auto conn = mconn.value();

        std::vector<bool> bf;
        //pieces is a byte string consisting of consecutive 20length SHA1 hashes
        //thus the number of pieces is the length of the byte string divided by 20
        int num_pieces = m_torrent->getMetaInfo().info.number_of_pieces();

        // bitfield must be a multiple of 8
        int mult8 = num_pieces % 8;
        int tail = mult8 == 0 ? 0 : 8 - mult8;
        //here we're essentially telling all others peers we have no pieces for them to download
        //this is to stop peers from requesting pieces from us
        //in the future I hope to enable this functionality
        for(int i = 0; i < num_pieces + tail ;i++) {
            bf.push_back(0);
        }
        auto msg = Bitfield(bf);
        auto msg_ptr = std::make_unique<Bitfield>(msg);

        BOOST_LOG(m_lg) << p.m_ip << " >>> " << msg.pprint();

        conn->write_message(
            std::move(msg_ptr)
            , boost::bind(&Client::sent_bitfield,this,p
                        ,boost::asio::placeholders::error
                        ,boost::asio::placeholders::bytes_transferred));
    }

    void Client::sent_bitfield(PeerId p,const boost_error &error,size_t size) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        auto conn = mconn.value();

        if(error) {
            BOOST_LOG(m_lg) << "[Client] sending bitfield to " << p.m_ip << " failed with " << error.message();
            conn->cancel();
            return;
        }
    }

    void Client::send_interested(PeerId p) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        auto conn = mconn.value();


        if(!m_peer_status[p].m_am_interested && m_peer_status[p].m_available_pieces.size() > 0) {
            auto msg_ptr = std::make_unique<Interested>(Interested());
            BOOST_LOG(m_lg) << p.m_ip << " >>> " << msg_ptr->pprint();
            conn->write_message(std::move(msg_ptr)
                ,boost::bind(&Client::sent_interested,this,p
                        ,boost::asio::placeholders::error
                        ,boost::asio::placeholders::bytes_transferred));
        }
    }

    void Client::sent_interested(PeerId p,const boost_error &error,size_t size) {
        auto mconn = m_peers.lookup(p);
        if(!mconn.has_value()) { return; }
        auto conn = mconn.value();

        if(error) {
            BOOST_LOG(m_lg) << "[Client] sending interested to " << p.m_ip << " failed with " << error.message();
            conn->cancel();
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
        auto mpeer = m_peers.lookup(p);
        if(!mpeer.has_value()) { return; }

        auto peer = mpeer.value();
        if(!m_peers.lookup_work(p).has_value()) {
            select_piece(p); // assign work to peer
        }
        auto mwork = m_peers.lookup_work(p);
        if(!mwork.has_value()) { 
            BOOST_LOG(m_lg) << "[Client] expected work to be assigned to peer " + p.m_ip;
            return;
        }
        auto work = mwork.value();

        //calculate size to request
        int standard_size = 1 << 14 ; // 16KB, standard request size 
        int remaining = work->m_data.remaining(); // remaining data of piece
        int piece_length = m_torrent->getMetaInfo().info.piece_length; //size of pieces
        //the size of the request cannot exceed any one of the above sizes
        int request_size = std::min(remaining,std::min(piece_length,request_size));

        Request request(work->m_data.m_piece_index
                        ,work->m_data.next_block_begin()
                        ,request_size);
        BOOST_LOG(m_lg) << p.m_ip + " >>> " + request.pprint();
        auto req_ptr = std::make_unique<Request>(request);
        peer->write_message(std::move(req_ptr)
            ,boost::bind(&Client::sent_piece_request,this,p
                        ,boost::asio::placeholders::error
                        ,boost::asio::placeholders::bytes_transferred));
    }

    void Client::sent_piece_request(PeerId p,const boost_error &error, size_t size) {
        if(error) {
            BOOST_LOG(m_lg) << "[Client] sending piece request to " << p.m_ip << " failed with " << error.message();
            return;
        }
        
        auto mwork = m_peers.lookup_work(p);
        if(!mwork.has_value()) {
            BOOST_LOG(m_lg) << "will segment fault due to progress " << p.m_ip;
            return;
        }

        auto mpeer = m_peers.lookup(p);
        if(!mpeer.has_value()) {
            BOOST_LOG(m_lg) << "will segment fault due to connections " << p.m_ip;
            return;
        }

        mwork.value()->m_progress = PieceProgress::Requested;

        auto &timer = mpeer.value()->get_timer();
        timer.async_wait(boost::posix_time::seconds(15)
            ,boost::bind(
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
}