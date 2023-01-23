#pragma once

#include "Event.h"

#include "fractals/common/WorkQueue.h"

namespace fractals::disk
{
    static constexpr uint32_t WORK_QUEUE_SIZE = 256;
    using DiskEventQueue = common::WorkQueueImpl<WORK_QUEUE_SIZE, DiskEvent>;
}