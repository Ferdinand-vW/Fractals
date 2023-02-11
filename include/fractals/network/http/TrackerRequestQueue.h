#pragma once

#include "fractals/common/WorkQueue.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::network::http
{
    static constexpr uint32_t WORK_QUEUE_SIZE = 256;
    using TrackerRequestQueue = common::WorkQueueImpl<WORK_QUEUE_SIZE, torrent::TorrentMeta>;
}