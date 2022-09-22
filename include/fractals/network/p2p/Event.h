#pragma once

#include "fractals/network/http/Peer.h"
#include <string>

namespace fractals::network::p2p
{
    struct PeerEvent
    {
    };

    struct EpollError : PeerEvent
    {
        std::string mEpollId;
        std::string mError;
    };

    struct ReceiveError : PeerEvent
    {
        std::string mError;
        http::PeerId mPeerId;
    };

    

}