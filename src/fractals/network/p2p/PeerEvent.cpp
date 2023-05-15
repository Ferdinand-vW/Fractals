#include "fractals/network/p2p/PeerEvent.h"

namespace fractals::network::p2p
{
    std::ostream &operator<<(std::ostream &os, const Message &m)
    {
        return os << "Message[peer=" << m.peer.toString() << ", content=" << m.message << "]";
    }

    std::ostream &operator<<(std::ostream &os, const Disconnect &d)
    {
        return os << "Disconnect[peer=" << d.peerId.toString() << "]";
    }

    std::ostream &operator<<(std::ostream &os, const Shutdown &e)
    {
        return os << "Shutdown";
    }
}