#pragma once

#include <string>
#include <sys/types.h>

class PeerId {
    public:
        std::string m_ip;
        uint m_port;

        PeerId(std::string ip,uint port);

        bool operator<(const PeerId &p2) const;
};

struct Peer {
    std::string peer_name;
    PeerId peer_id;
};