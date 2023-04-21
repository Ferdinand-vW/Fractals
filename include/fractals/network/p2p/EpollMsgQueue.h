#pragma once

#include "fractals/common/FullDuplexQueue.h"
#include "fractals/common/WorkQueue.h"
#include "fractals/network/p2p/EpollServiceEvent.h"

#include <cstdint>

namespace fractals::network::p2p
{
    static constexpr uint32_t EPOLL_REQUEST_QUEUE_SIZE = 256;
    using EpollMsgQueue = common::FullDuplexQueue<EPOLL_REQUEST_QUEUE_SIZE, EpollServiceRequest, EpollServiceResponse>;
}