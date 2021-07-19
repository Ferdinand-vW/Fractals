#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <set>

#include <boost/log/sources/logger.hpp>

#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/PeerWork.h"
#include "fractals/torrent/Torrent.h"

namespace boost { namespace asio { class io_context; } }
namespace fractals::persist { class Storage; }
namespace fractals::torrent { class Torrent; }

namespace fractals::network::p2p {

    class Client;

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
        int m_max_peers = 20;
        
        boost::asio::io_context &m_io;
        std::recursive_mutex m_mutex;

        boost::log::sources::logger_mt &m_lg;

        persist::Storage &m_storage;


        public:
            BitTorrent (std::shared_ptr<torrent::Torrent> t
                    ,boost::asio::io_context &io
                    ,persist::Storage &st);
            void run();
            void stop();

            std::shared_ptr<torrent::Torrent> m_torrent;
            std::shared_ptr<Client> m_client;
            std::set<http::PeerId> m_available_peers;
            int m_leecher_count;
            int connected_peers();
            int available_peers();
            int known_leecher_count();

        private:

            void request_peers();
            std::optional<http::PeerId> choose_peer();
            void setup_client();
            void attempt_connect(http::PeerId p);
            void peer_change(http::PeerId p,PeerChange pc);
            void connect_to_a_peer();

            void perform_handshake(http::PeerId p);
    };


}