#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/sources/logger.hpp>
#include <condition_variable>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <boost/asio.hpp>

#include "network/p2p/Connection.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/Message.h"
#include "network/http/Tracker.h"
#include "network/p2p/Response.h"
#include "common/logger.h"
#include "torrent/Torrent.h"
#include "torrent/PieceData.h"

using namespace boost::asio;

struct P2PStatus {
    bool m_am_choking = true;
    bool m_am_interested = false;
    bool m_peer_choking = true;
    bool m_peer_interested = false;
    std::set<int> m_available_pieces;
};

enum class PieceProgress { Nothing, Requested, Downloaded, Completed };
enum class PeerChange { Added, Removed };
struct PieceStatus {
    PieceProgress m_progress;
    PieceData m_data;
    long long offset; // Use offset to determine from which point to request block 
};


class Client : public enable_shared_from_this<Client> {
    std::map<PeerId,P2PStatus> m_peer_status;

    //a piece must always be present in at least one of these 3 containers
    std::set<int> m_missing_pieces;
    std::set<int> m_existing_pieces;
    std::map<PeerId, std::unique_ptr<PieceStatus>> m_progress;

    boost::asio::io_context& m_io;
    std::map<PeerId, std::shared_ptr<Connection>> m_connections;

    std::shared_ptr<Torrent> m_torrent;
    std::unique_ptr<std::mutex> m_piece_lock;
    
    std::function<void(PeerId,PeerChange)> m_on_change_peers;

    boost::log::sources::logger_mt &m_lg;

    public:
        std::vector<char> m_client_id;
        
        Client(std::shared_ptr<Torrent> torrent
              ,boost::asio::io_context &io
              ,std::function<void(PeerId,PeerChange)> on_change_peers);

        bool has_all_pieces();
        bool is_choked_by(PeerId p);
        bool is_connected_to(PeerId p);
        void connect_to_peer(PeerId p);
        void connected(PeerId,const boost_error &error);
        void drop_connection(PeerId p);

        void await_handshake(PeerId p);
        void await_messages(PeerId p);

        void received_choke(PeerId p);
        void received_unchoke(PeerId p);
        void received_interested(PeerId p);
        void received_not_interested(PeerId p);
        void received_have(PeerId p, Have &h);
        void received_bitfield(PeerId p,Bitfield &bf);
        void received_request(PeerId p,Request &r);
        void received_piece(PeerId p,Piece &pc);
        void received_cancel(PeerId p, Cancel c);
        void received_garbage(PeerId p);
        // void received_port(PeerId p,Port port)

        void write_messages(PeerId p);
        void send_handshake(PeerId p,HandShake &&hs);
    
        void send_messages(PeerId p);
        
        void add_peer(PeerId p);

    private:
        void add_peer_progress(PeerId p);

        void send_piece_requests(PeerId p);
        void sent_piece_request(PeerId p,const boost_error &error, size_t size);
        void piece_response_timeout(PeerId p,const boost_error &error);
        
        void send_bitfield(PeerId p);
        void sent_bitfield(PeerId p,const boost_error &error,size_t size);
        
        void send_interested(PeerId p);
        void sent_interested(PeerId p,const boost_error &error,size_t size);

        void wait_for_unchoke(PeerId p);
        void unchoke_timeout(PeerId p,const boost_error & error);

        void handle_peer_message(PeerId p,const boost_error &error,int length,std::deque<char> &&deq_buf);
        void handle_peer_handshake(PeerId p,const boost_error &error,int lenght,std::deque<char> &&deq_buf);
        void handshake_timeout(PeerId,const boost_error &error);
        void connect_timeout(PeerId,const boost_error &error);
        void select_piece(PeerId p);
};
