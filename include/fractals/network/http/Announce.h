#pragma once

#include <bits/types/time_t.h>
#include <vector>
#include <optional>

#include "fractals/network/http/Peer.h"

namespace fractals::network::http {

    /**
    Model of Announce as returned by a tracker
    */
    struct Announce {
        time_t announce_time;
        int interval;
        std::optional<int> min_interval;
        std::vector<PeerId> peers;
    };

}