#pragma once

#include "network/http/Peer.h"
#include "network/p2p/Connection.h"
#include "network/p2p/PeerWork.h"

#include <mutex>

class PeerManager {
    int m_max_conn;
    bool m_enabled = true;
    std::map<PeerId,std::shared_ptr<Connection>> m_conns;
    std::map<PeerId, std::shared_ptr<PeerWork>> m_work;
    std::mutex m_mutex;

    public:
        PeerManager(int max_conn);

        //update connections
        bool add_new_connection(PeerId p,std::shared_ptr<Connection> conn);
        void remove_connection(PeerId p);
        
        //disable or enable the connection manager
        void disable(); //stop accepting new connections and drop current ones
        void enable(); //accept new connections again
        bool is_enabled();

        void new_work(PeerId p,std::shared_ptr<PeerWork> work);
        std::optional<std::shared_ptr<PeerWork>> lookup_work(PeerId p);
        void finished_work(PeerId p);

        //queries
        bool is_connected_to(PeerId p);
        std::optional<std::shared_ptr<Connection>> lookup(PeerId p);
        std::shared_ptr<Connection> const& operator[](PeerId p);
};