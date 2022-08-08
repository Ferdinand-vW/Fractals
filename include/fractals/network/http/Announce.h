#pragma once

#include "Peer.h"

#include <optional>
#include <vector>

namespace fractals::network::http
{

    /**
    Model of Announce as returned by a tracker
    */
    struct Announce {
        time_t announce_time;
        int interval;
        std::optional<int> min_interval;
        std::vector<PeerId> peers;

        bool operator==(const Announce& ann) const;
    };
}