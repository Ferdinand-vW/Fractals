#pragma once

#include "Socket.h"

#include <cstdint>

namespace fractals::network::p2p
{
    struct PeerFd
    {
        std::string mHost;
        uint16_t mPort;

        Socket mSocket;

        int32_t getFileDescriptor()
        {
            return mSocket.getFileDescriptor();
        }
    };
}