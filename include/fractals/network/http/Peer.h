#pragma once

#include <string>
#include <sys/types.h>

namespace fractals::network::http {

    /**
    A peer is identified by ip address and port
    */
    class PeerId {
        public:
            std::string m_ip;
            uint m_port;

            PeerId(std::string ip,uint port);

            //Required for ordered data structures
            bool operator<(const PeerId &p2) const;
            bool operator==(const PeerId &p) const;
    };

    /**
    A Peer is also assigned some random unique name
    */
    struct Peer {
        std::string peer_name;
        PeerId peer_id;

        bool operator==(const Peer& p) const;
    };

}