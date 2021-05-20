#pragma once

#include <bits/types/time_t.h>
#include <vector>

#include "PeerId.h"

struct Announce {
    time_t datetime;
    std::vector<PeerId> peers;
};