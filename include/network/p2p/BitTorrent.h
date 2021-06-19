#pragma once

#include "network/p2p/Connection.h"
#include "persist/storage.h"
#include "persist/data.h"
#include "torrent/Torrent.h"
#include "network/p2p/Client.h"
#include "network/http/Peer.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/log/sources/logger.hpp>
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
    int m_max_peers = 7;
    
    boost::asio::io_context &m_io;
    std::recursive_mutex m_mutex;

    boost::log::sources::logger_mt &m_lg;

    Storage m_storage;


    public:
        BitTorrent (std::shared_ptr<Torrent> t
                   ,boost::asio::io_context &io
                   ,Storage st);
        void run();
        void stop();

        std::shared_ptr<Torrent> m_torrent;
        std::shared_ptr<Client> m_client;
        std::set<PeerId> m_available_peers;
        std::atomic<int> m_connected = 0;

    private:
        void request_peers();
        std::optional<PeerId> choose_peer();
        void setup_client();
        void attempt_connect(PeerId p);
        void peer_change(PeerId p,PeerChange pc);
        std::optional<PeerId> connect_to_a_peer();

        void perform_handshake(PeerId p);
};
