#include "fractals/common/utils.h"
#include "fractals/network/p2p/Event.h"

namespace fractals::network::p2p
{
    std::ostream& operator<<(std::ostream& os, const EpollError &e)
    {
        return os << "EpollError";
    };
    
    std::ostream& operator<<(std::ostream& os, const ReceiveError &e)
    {
        return os << "ReceiveError";
    };

    std::ostream& operator<<(std::ostream& os, const ReceiveEvent &e)
    {
        return os << "ReceiveEvent: " << e.mMessage;
    };
    
    std::ostream& operator<<(std::ostream& os, const ConnectionCloseEvent &e)
    {
        return os << "ConnectionCloseEvent";
    };
    
    std::ostream& operator<<(std::ostream& os, const ConnectionError &e)
    {
        return os << "ConnectionError";
    };

    std::ostream& operator<<(std::ostream& os, const PeerEvent &pe)
    {
        std::visit(common::overloaded {
            [&](const auto& v) { os << v; }
        }, pe);

        return os;
    }
}