#pragma once

#include <condition_variable>
#include <fstream>
#include <set>
#include <boost/asio.hpp>

#include "network/p2p/PeerId.h"
#include "network/p2p/Message.h"
#include "network/http/Tracker.h"
#include "torrent/Torrent.h"
#include "torrent/PieceData.h"

using namespace boost::asio;
using ip::tcp;

struct P2PStatus {
    bool m_am_choking = true;
    bool m_am_interested = false;
    bool m_peer_choking = true;
    bool m_peer_interested = false;
    std::set<int> m_available_pieces;
};

enum class PieceProgress { Nothing, Requested, Downloaded, Completed };
struct PieceStatus {
    PieceProgress m_progress;
    PieceData m_data;
};


class Client {
    std::map<PeerId,P2PStatus> m_peer_status;

    std::set<int> m_missing_pieces;
    std::set<int> m_existing_pieces;

    std::shared_ptr<tcp::socket> m_socket;

    std::unique_ptr<std::mutex> m_request_mutex;
    std::unique_ptr<std::condition_variable> m_request_cv;

    std::shared_ptr<Torrent> m_torrent;


    std::unique_ptr<PieceStatus> cur_piece;
    
    public:
        std::vector<char> m_client_id;
        
        Client(std::unique_ptr<std::mutex> request_mutex,std::unique_ptr<std::condition_variable> request_cv
              ,std::shared_ptr<tcp::socket> socket
              ,std::shared_ptr<Torrent> torrent);

        bool connect_peer(PeerId p);
        bool has_all_pieces();
        bool is_choked_by(PeerId p);

        void received_choke(PeerId p);
        void received_unchoke(PeerId p);
        void received_interested(PeerId p);
        void received_not_interested(PeerId p);
        void received_have(PeerId p, int piece);
        void received_bitfield(PeerId p,Bitfield &bf);
        void received_request(PeerId p,Request r);
        void received_piece(PeerId p,Piece pc);
        void received_cancel(PeerId p, Cancel c);
        void received_garbage(PeerId p);
        // void received_port(PeerId p,Port port)

        void send_handshake(const HandShake &hs);
        void send_messages(PeerId p);
        void send_interested(PeerId p);
        void send_piece_request(PeerId p);

        void add_peer(PeerId p);

    private:
        void sent_interested(PeerId p);
};
