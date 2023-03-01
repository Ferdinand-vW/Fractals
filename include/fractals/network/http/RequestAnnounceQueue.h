#pragma once

#include "fractals/common/FullDuplexQueue.h"
#include "fractals/common/WorkQueue.h"
#include "fractals/network/http/Request.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::network::http
{
    static constexpr uint32_t WORK_QUEUE_SIZE = 256;
    using RequestAnnounceQueue = common::FullDuplexQueue<WORK_QUEUE_SIZE, TrackerRequest, Announce>;
}