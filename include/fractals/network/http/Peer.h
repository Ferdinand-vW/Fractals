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
    std::string ip{""};
    uint16_t port{0};

    constexpr PeerId() = default;
    constexpr PeerId(const std::string& ip, uint16_t port) : ip(ip), port(port){};

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
    std::string name;
    PeerId id;

    bool operator==(const Peer &p) const;
};

} // namespace fractals::network::http

namespace std
{
template <> struct std::hash<fractals::network::http::PeerId>
{
    std::size_t operator()(fractals::network::http::PeerId p) const
    {

        // Compute individual hash values for first,
        // second and third and combine them using XOR
        // and bit shifting:

        return ((std::hash<std::string>()(p.ip) ^ (std::hash<uint>()(p.port) << 1)) >> 1);
    }
};
} // namespace std