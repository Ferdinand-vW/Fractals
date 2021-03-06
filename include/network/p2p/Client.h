#pragma once

#include <set>
#include <boost/asio.hpp>

#include "network/p2p/PeerId.h"
#include "network/p2p/Message.h"
#include "network/http/Tracker.h"

using namespace boost::asio;
using ip::tcp;

struct P2PStatus {
    bool m_am_choking = true;
    bool m_am_interested = false;
    bool m_peer_choking = true;
    bool m_peer_interested = false;
};

class Client {
    std::map<PeerId,P2PStatus> m_peer_status;

    std::set<int> pieces;


    
    public:
        Client(tcp::socket &socket);

        bool connect_peer(PeerId p);

        void received_choke(PeerId p);
        void received_unchoke(PeerId p);
        void received_interested(PeerId p);
        void received_not_interested(PeerId p);
        void received_keep_alive();
        void received_have(PeerId p, int piece);
        void received_bitfield(PeerId p,Bitfield &bf);
        void received_request(PeerId p,Request r);
        void received_piece(PeerId p,Piece pc);
        void received_cancel(PeerId p, Cancel c);
        // void received_port(PeerId p,Port port)

        void add_peer(PeerId p);
};
