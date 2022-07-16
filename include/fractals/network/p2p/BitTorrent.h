#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <set>

#include <boost/log/sources/logger.hpp>

#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/PeerWork.h"
#include "fractals/torrent/Torrent.h"
#include "AnnounceService.h"

namespace boost { namespace asio { class io_context; } }
namespace fractals::persist { class Storage; }
namespace fractals::torrent { class Torrent; }

namespace fractals::network::p2p {

    class Client;

   /**
   This class represents the BitTorrent protocol for a particular Torrent.
   It manages connections to the different peers and handles the writing the data to file
   in addition to persisting the state for the torrent. The state includes which pieces have been
   downloaded and which peers are known to have data for the relevant torrent.

   Note that the class currently only supports leeching. Seeder functionality is not yet available.
   */
    class BitTorrent {
        int m_max_peers = 20;
        
        boost::asio::io_context &m_io;
        std::mutex mMutex;

        boost::log::sources::logger_mt &m_lg;

        persist::Storage &m_storage;

        AnnounceService mAnnService;


        public:
            BitTorrent (std::shared_ptr<torrent::Torrent> t
                    ,boost::asio::io_context &io
                    ,persist::Storage &st);

            /**
            Start downloading
            */
            void run();

            /**
            Stop downloading
            */
            void stop();

            /**
            The Torrent for which we wish to engage in the BitTorrent protocol
            */
            std::shared_ptr<torrent::Torrent> m_torrent;

            /**
            This app
            */
            std::shared_ptr<Client> m_client;

            /**
            Known peers after announce received from tracker
            */
            std::set<http::PeerId> m_available_peers;
            
            /**
            Number of connected leechers
            */
            int m_leecher_count;
            
            /**
            Number of connected seeders
            */
            int connected_peers();

            /**
            Equal to m_available_peers.size()
            */
            int available_peers();

            /**
            Number of known leechers (returned by announce)
            */
            int known_leecher_count() const;

        private:

            /**
            Pick a peer from the announce set. If announce set is empty then perform a new announce.
            */
            void request_peers();
            std::optional<http::PeerId> choose_peer();

            /**
            Construct client object and load previously downloaded pieces
            */
            void setup_client();

            /**
            Make the client try to connect to the given peer
            */
            void attempt_connect(http::PeerId p);

            /**
            Callback function for when client either successfully connected to peer
            or had to drop a peer.
            */
            void peer_change(http::PeerId p,PeerChange pc);

            /**
            Connect to a new peer from the announce set
            */
            void connect_to_a_peer();

            /**
            After succesfull connection we need to initiate handshake.
            Client will handle rest.
            */
            void perform_handshake(http::PeerId p);
    };


}