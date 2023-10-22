#include "fractals/network/http/Peer.h"

namespace fractals::network::http
{
bool PeerId::operator<(const PeerId &p2) const
{
    return ip < p2.ip || (ip == p2.ip && port < p2.port);
}

bool PeerId::operator==(const PeerId &p) const
{
    return ip == p.ip && port == p.port;
}

std::string PeerId::toString() const
{
    return ip + ":" + std::to_string(port);
}

bool Peer::operator==(const Peer &p) const
{
    return id == p.id && name == p.name;
}

} // namespace fractals::network::http