#pragma once

#include "Event.h"

#include "fractals/common/FullDuplexQueue.h"
#include "fractals/common/WorkQueue.h"

namespace fractals::app
{
    static constexpr uint32_t WORK_QUEUE_SIZE = 256;
    using AppEventQueue = common::FullDuplexQueue<WORK_QUEUE_SIZE, ResponseToApp, RequestFromApp>;
}