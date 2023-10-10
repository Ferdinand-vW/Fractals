#pragma once

#include "Peer.h"
#include "fractals/common/Tagged.h"

#include <optional>
#include <vector>

namespace fractals::network::http
{

    /**
    Model of Announce as returned by a tracker
    */
    struct Announce {
        common::InfoHash infoHash;
        time_t announce_time;
        int interval;
        std::optional<int> min_interval;
        std::vector<PeerId> peers;

        bool operator==(const Announce& ann) const;
    };
}