#pragma once

#include "fractals/common/WorkQueue.h"
#include "fractals/network/p2p/Event.h"

namespace fractals::network::p2p
{
    static constexpr uint32_t WORK_QUEUE_SIZE = 256;
    using PeerEventQueue = common::WorkQueueImpl<WORK_QUEUE_SIZE, PeerEvent>;
}