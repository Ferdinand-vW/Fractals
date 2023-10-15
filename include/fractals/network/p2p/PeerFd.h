#pragma once

#include "Socket.h"
#include "fractals/network/http/Peer.h"

#include <cstdint>

namespace fractals::network::p2p
{
class PeerFd
{
  public:
    constexpr PeerFd() = default;
    constexpr PeerFd(const http::PeerId &id, int32_t fd) : mId(id), mFd(fd)
    {
    }

    int32_t getFileDescriptor() const
    {
        return mFd;
    }

    http::PeerId getId() const
    {
        return mId;
    }

    bool operator==(const PeerFd &p) const
    {
        return mId == p.getId();
    }

    static PeerFd invalid()
    {
        return PeerFd{http::PeerId{"", 0}, 0};
    }

  private:
    http::PeerId mId;
    int32_t mFd;
};
} // namespace fractals::network::p2p

namespace std
{
template <> struct std::hash<fractals::network::p2p::PeerFd>
{
    std::size_t operator()(const fractals::network::p2p::PeerFd &p) const
    {

        // Compute individual hash values for first,
        // second and third and combine them using XOR
        // and bit shifting:
        std::hash<fractals::network::http::PeerId> h;
        return h(p.getId());
    }
};
} // namespace std