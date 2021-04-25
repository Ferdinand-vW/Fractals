#pragma once

#include "network/p2p/Connection.h"
#include "network/p2p/Response.h"
#include "torrent/Torrent.h"
#include "network/p2p/Client.h"
#include "network/p2p/PeerListener.h"
#include "network/p2p/PeerId.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
/*

One Client class per Torrent
One PeerListener per Peer
Multiple Peer per Torrent
Peer has only one Torrent

One Client may receive messages from or send messages to multiple peers

PeerListener should constinuously listen to messages sent by associated Peer
Messages should be forwarded to the corresponding Client

(Torrent identifier has name of torrent,file names and location on disk)
Protocol should accept torrent identifier, present pieces and a list of peers
1) If 

*/
class BitTorrent {
    std::shared_ptr<Client> m_client;
    std::set<PeerId> m_available_peers;

    int m_max_peers = 1;
    int m_connected = 0;
    
    std::shared_ptr<Torrent> m_torrent;
    boost::asio::io_context &m_io;
    // std::mutex m_mutex;


    public:
        BitTorrent (std::shared_ptr<Torrent> t,boost::asio::io_context &io);
        void run();

    private:
        void request_peers();
        PeerId choose_peer();
        void setup_client();
        void attempt_connect(PeerId p);
        void peer_change(PeerId p,PeerChange pc);
        PeerId connect_to_a_peer();
        PeerId connect_and_handshake();

        FutureResponse perform_handshake(PeerId p);
};
