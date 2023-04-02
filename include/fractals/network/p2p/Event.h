#pragma once

#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"

#include <deque>
#include <epoll_wrapper/Error.h>
#include <ostream>
#include <string>
#include <variant>

namespace fractals::network::p2p
{
    struct EpollError
    {
        epoll_wrapper::ErrorCode errorCode;
    };

    std::ostream& operator<<(std::ostream& os, const EpollError &e);

    struct ReceiveError
    {
        http::PeerId peerId;
        epoll_wrapper::ErrorCode errorCode;
    };

    std::ostream& operator<<(std::ostream& os, const ReceiveError &e);

    struct ReceiveEvent
    {
        http::PeerId peerId;
        BitTorrentMessage mMessage;
    };

    std::ostream& operator<<(std::ostream& os, const ReceiveEvent &e);

    struct ConnectionCloseEvent
    {
        http::PeerId peerId;
    };

    std::ostream& operator<<(std::ostream& os, const ConnectionCloseEvent &e);

    struct ConnectionError
    {
        http::PeerId peerId;
        epoll_wrapper::ErrorCode errorCode;
    };
    
    std::ostream& operator<<(std::ostream& os, const ConnectionError &e);

    using PeerEvent = std::variant
        <EpollError
        ,ReceiveError
        ,ReceiveEvent
        ,ConnectionCloseEvent
        ,ConnectionError>;

    std::ostream& operator<<(std::ostream& os, const PeerEvent &pe);
}