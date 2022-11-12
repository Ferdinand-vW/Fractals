#pragma once

#include "fractals/network/http/Peer.h"

#include <deque>
#include <epoll_wrapper/Error.h>
#include <string>

namespace fractals::network::p2p
{
    struct PeerEvent
    {
    };

    struct EpollError : PeerEvent
    {
        std::string mError;
    };

    struct ReceiveError : PeerEvent
    {
        std::string mError;
        http::PeerId mPeerId;
    };

    struct ReceiveEvent : PeerEvent
    {
        http::Peer mPeerId;
        std::deque<char> mData;
    };

    struct ConnectionCloseEvent : PeerEvent
    {
        http::Peer mPeerId;
    };

    struct ConnectionError : PeerEvent
    {
        http::Peer mPeerId;
        epoll_wrapper::ErrorCode mError;
    };
}