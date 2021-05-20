#pragma once

#include <bits/types/time_t.h>

struct AnnounceModel {
    int id;
    int torrent_id;
    time_t datetime;
};