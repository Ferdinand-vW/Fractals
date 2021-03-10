#pragma once

#include <string>

class PeerId {
    public:
        std::string m_ip;
        uint m_port;

        PeerId(std::string ip,uint port);

        bool operator<(const PeerId &p2) const;

};