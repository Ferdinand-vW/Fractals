#pragma once

#include "fractals/network/http/Peer.h"

#include <deque>
#include <epoll_wrapper/Error.h>
#include <ostream>
#include <string>
#include <variant>

namespace fractals::network::p2p
{
    struct EpollError
    {
        epoll_wrapper::ErrorCode mError;
    };

    std::ostream& operator<<(std::ostream& os, const EpollError &e);

    struct ReceiveError
    {
        epoll_wrapper::ErrorCode mError;
        http::PeerId mPeerId;
    };

    std::ostream& operator<<(std::ostream& os, const ReceiveError &e);

    struct ReceiveEvent
    {
        http::Peer mPeerId;
        std::deque<char> mData;
    };

    std::ostream& operator<<(std::ostream& os, const ReceiveEvent &e);

    struct ConnectionCloseEvent
    {
        http::Peer mPeerId;
    };

    std::ostream& operator<<(std::ostream& os, const ConnectionCloseEvent &e);

    struct ConnectionError
    {
        http::Peer mPeerId;
        epoll_wrapper::ErrorCode mError;
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