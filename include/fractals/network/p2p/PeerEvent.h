#pragma once

#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/PeerFd.h"

#include <deque>
#include <epoll_wrapper/Error.h>
#include <ostream>
#include <string>
#include <variant>

namespace fractals::network::p2p
{

struct ConnectionEstablished
{
    http::PeerId peer;
};

struct Message
{
    http::PeerId peer;
    BitTorrentMessage message;

    bool operator==(const Message &obj) const
    {
        return peer == obj.peer && message == obj.message;
    }
};

std::ostream &operator<<(std::ostream &os, const Message &e);

struct ConnectionDisconnected
{
    http::PeerId peerId;

    bool operator==(const ConnectionDisconnected &obj) const
    {
        return peerId == obj.peerId;
    }
};

std::ostream &operator<<(std::ostream &os, const ConnectionDisconnected &e);

struct Shutdown
{
    bool operator==(const Shutdown &obj) const
    {
        return true;
    }
};

std::ostream &operator<<(std::ostream &os, const Shutdown &e);


using PeerEvent = std::variant<ConnectionEstablished, Message, ConnectionDisconnected, Shutdown>;

std::ostream &operator<<(std::ostream &os, const PeerEvent &pe);
} // namespace fractals::network::p2p