#pragma once

#include "Event.h"

#include "fractals/common/WorkQueue.h"

namespace fractals::persist
{
    static constexpr uint32_t WORK_QUEUE_SIZE = 256;
    using StorageEventQueue = common::WorkQueueImpl<WORK_QUEUE_SIZE, StorageEvent>;
}