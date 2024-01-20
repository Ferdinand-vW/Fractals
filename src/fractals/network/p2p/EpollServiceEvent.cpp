#include <fractals/common/utils.h>
#include <fractals/network/p2p/BitTorrentMsg.h>
#include <fractals/network/p2p/EpollServiceEvent.h>

namespace fractals::network::p2p
{
std::ostream &operator<<(std::ostream &os, const EpollError &e)
{
    return os << "EpollError";
};

std::ostream &operator<<(std::ostream &os, const ReadEventResponse &e)
{
    return os << "ReceiveError";
};

std::ostream &operator<<(std::ostream &os, const ReadEvent &e)
{
    return os << "ReadEvent: bytes=" << e.mMessage.size();
};

std::ostream &operator<<(std::ostream &os, const WriteEvent &e)
{
    return os << "WriteEvent: "
              << "peer=" << e.peer.getId().toString() << ", bytes=" << e.message.size();
};

std::ostream &operator<<(std::ostream &os, const WriteEventResponse &e)
{
    return os << "WriteEventResponse: "
              << "peer=" << e.peer.getId().toString() << ", error=" << e.errorMsg;
};

std::ostream &operator<<(std::ostream &os, const Subscribe &e)
{
    return os << "Subscribe: "
              << "peer=" << e.peer.getId().toString();
};

std::ostream &operator<<(std::ostream &os, const UnSubscribe &e)
{
    return os << "UnSubscribe: "
              << "peer=" << e.peer.getId().toString();
};

std::ostream &operator<<(std::ostream &os, const CtlResponse &e)
{
    return os << "CtlResponse: "
              << "peer=" << e.peer.getId().toString() << " error=" << e.errorMsg;
};

std::ostream &operator<<(std::ostream &os, const ConnectionCloseEvent &e)
{
    return os << "ConnectionCloseEvent";
};

std::ostream &operator<<(std::ostream &os, const ConnectionError &e)
{
    return os << "ConnectionError";
};

std::ostream &operator<<(std::ostream &os, const Deactivate &msg)
{
    return os << "Deactivate{}";
}

std::ostream &operator<<(std::ostream &os, const EpollServiceRequest &pe)
{
    std::visit(common::overloaded{[&](const auto &v) { os << v; }}, pe);

    return os;
}

std::ostream &operator<<(std::ostream &os, const EpollServiceResponse &pe)
{
    std::visit(common::overloaded{[&](const auto &v) { os << v; }}, pe);

    return os;
}
} // namespace fractals::network::p2p