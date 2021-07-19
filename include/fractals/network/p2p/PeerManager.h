#pragma once

#include <mutex>
#include <map>
#include <memory>
#include <optional>

#include "fractals/network/http/Peer.h"

namespace fractals::network::p2p {

    class Connection;
    class PeerWork;

    class PeerManager {
        int m_max_conn;
        bool m_enabled = true;
        std::map<http::PeerId,std::shared_ptr<Connection>> m_conns;
        std::map<http::PeerId,std::shared_ptr<PeerWork>> m_work;
        std::mutex m_mutex;

        public:
            PeerManager(int max_conn);

            //update connections
            bool add_new_connection(http::PeerId p,std::shared_ptr<Connection> conn);
            void remove_connection(http::PeerId p);
            
            //disable or enable the connection manager
            void disable(); //stop accepting new connections and drop current ones
            void enable(); //accept new connections again
            bool is_enabled();

            void new_work(http::PeerId p,std::shared_ptr<PeerWork> work);
            std::optional<std::shared_ptr<PeerWork>> lookup_work(http::PeerId p);
            void finished_work(http::PeerId p);

            //queries
            int num_connections();
            bool is_connected_to(http::PeerId p);
            std::optional<std::shared_ptr<Connection>> lookup(http::PeerId p);
            std::shared_ptr<Connection> const& operator[](http::PeerId p);
    };

}