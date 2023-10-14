#include "fractals/network/http/Peer.h"

namespace fractals::network::http {
    bool PeerId::operator<(const PeerId &p2) const {
        return m_ip < p2.m_ip || (m_ip == p2.m_ip && m_port < p2.m_port);
    }

    bool PeerId::operator==(const PeerId &p) const
    {
        return m_ip == p.m_ip
            && m_port == p.m_port;
    }

    std::string PeerId::toString() const
    {
        return m_ip+":"+std::to_string(m_port);
    }

    bool Peer::operator==(const Peer &p) const
    {
        return peer_id == p.peer_id
            && peer_name == p.peer_name;
    }

}