#pragma once

#include <bits/types/time_t.h>
#include <vector>
#include <optional>

#include "Peer.h"

struct Announce {
    time_t announce_time;
    int interval;
    std::optional<int> min_interval;
    std::vector<PeerId> peers;
};