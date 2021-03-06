#include "fractals/network/http/Peer.h"

namespace fractals::network::http {

    PeerId::PeerId(std::string ip,uint port) : m_ip(ip),m_port(port) {};

    bool PeerId::operator<(const PeerId &p2) const {
        return m_ip < p2.m_ip || (m_ip == p2.m_ip && m_port < p2.m_port);
    }

}