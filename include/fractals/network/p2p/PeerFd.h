#pragma once

#include "Socket.h"
#include "fractals/network/http/Peer.h"

#include <cstdint>

namespace fractals::network::p2p
{
    struct PeerFd
    {
        http::PeerId mId;

        Socket mSocket;

        int32_t getFileDescriptor() const
        {
            return mSocket.getFileDescriptor();
        }
        
        http::PeerId getId() const
        {
            return mId;
        }
    };
}