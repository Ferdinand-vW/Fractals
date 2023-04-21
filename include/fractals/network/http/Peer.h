#pragma once

#include <string>
#include <sys/types.h>

namespace fractals::network::http
{

/**
A peer is identified by ip address and port
*/
class PeerId
{
  public:
    std::string m_ip{""};
    uint16_t m_port{0};

    PeerId() = default;
    PeerId(std::string ip, uint16_t port);

    // Required for ordered data structures
    bool operator<(const PeerId &p2) const;
    bool operator==(const PeerId &p) const;
    std::string toString() const;
};

/**
A Peer is also assigned some random unique name
*/
struct Peer
{
    std::string peer_name;
    PeerId peer_id;

    bool operator==(const Peer &p) const;
};

} // namespace fractals::network::http

namespace std
{
template <> struct std::hash<fractals::network::http::PeerId>
{
    std::size_t operator()(const fractals::network::http::PeerId &p) const
    {

        // Compute individual hash values for first,
        // second and third and combine them using XOR
        // and bit shifting:

        return ((std::hash<std::string>()(p.m_ip) ^ (std::hash<uint>()(p.m_port) << 1)) >> 1);
    }
};
} // namespace std