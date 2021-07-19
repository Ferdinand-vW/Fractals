#pragma once

#include <boost/log/sources/logger.hpp>
#include <map>
#include <memory>
#include <set>

#include "fractals/network/p2p/Connection.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/PeerManager.h"
#include "fractals/network/p2p/PeerWork.h"

namespace fractals::persist { class Storage; }
namespace fractals::torrent { class Torrent; }

namespace fractals::network::p2p {

    using namespace boost::asio;

    class HandShake;
    class Have;
    class Bitfield;
    class Request;
    class Piece;
    class Cancel;

    struct P2PStatus {
        bool m_am_choking = true;
        bool m_am_interested = false;
        bool m_peer_choking = true;
        bool m_peer_interested = false;
        std::set<int> m_available_pieces;
    };

    class Client : public std::enable_shared_from_this<Client> {
        std::map<http::PeerId,P2PStatus> m_peer_status;

        //a piece must always be present in at least one of these 3 containers
        std::set<int> m_missing_pieces;
        std::set<int> m_existing_pieces;
        std::unique_ptr<std::mutex> m_piece_lock;

        boost::asio::io_context& m_io;
        PeerManager m_peers;

        std::shared_ptr<torrent::Torrent> m_torrent;
        persist::Storage &m_storage;
        
        
        std::function<void(http::PeerId,PeerChange)> m_on_change_peers;

        boost::log::sources::logger_mt &m_lg;

        public:
            std::vector<char> m_client_id;
            
            Client(int max_peers
                ,std::shared_ptr<torrent::Torrent> torrent
                ,boost::asio::io_context &io
                ,persist::Storage &storage
                ,std::function<void(http::PeerId,PeerChange)> on_change_peers);

            void close_connections();


            bool is_enabled();
            bool has_all_pieces();
            bool is_choked_by(http::PeerId p);
            bool is_connected_to(http::PeerId p);
            int num_connections();
            void connect_to_peer(http::PeerId p);
            void connected(http::PeerId,const boost_error &error);
            void drop_connection(http::PeerId p);

            void await_handshake(http::PeerId p);
            void await_messages(http::PeerId p);

            void received_choke(http::PeerId p);
            void received_unchoke(http::PeerId p);
            void received_interested(http::PeerId p);
            void received_not_interested(http::PeerId p);
            void received_have(http::PeerId p, Have &h);
            void received_bitfield(http::PeerId p,Bitfield &bf);
            void received_request(http::PeerId p,Request &r);
            void received_piece(http::PeerId p,Piece &pc);
            void received_cancel(http::PeerId p, Cancel c);
            void received_garbage(http::PeerId p);
            // void received_port(http::PeerId p,Port port)

            void write_messages(http::PeerId p);
            void send_handshake(http::PeerId p,HandShake &&hs);
        
            void send_messages(http::PeerId p);
            
            void add_peer(http::PeerId p);

        private:
            void add_peer_work(http::PeerId p,int piece, long long piece_size);

            void send_piece_requests(http::PeerId p);
            void sent_piece_request(http::PeerId p,const boost_error &error, size_t size);
            void piece_response_timeout(http::PeerId p,const boost_error &error);
            
            void send_bitfield(http::PeerId p);
            void sent_bitfield(http::PeerId p,const boost_error &error,size_t size);
            
            void send_interested(http::PeerId p);
            void sent_interested(http::PeerId p,const boost_error &error,size_t size);

            void wait_for_unchoke(http::PeerId p);
            void unchoke_timeout(http::PeerId p,const boost_error & error);

            void handle_peer_message(http::PeerId p,const boost_error &error,int length,std::deque<char> &&deq_buf);
            void handle_peer_handshake(http::PeerId p,const boost_error &error,int lenght,std::deque<char> &&deq_buf);
            void handshake_timeout(http::PeerId,const boost_error &error);
            void connect_timeout(http::PeerId,const boost_error &error);
            void select_piece(http::PeerId p);
    };

}