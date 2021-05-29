#pragma once

#include <string>
#include <bits/types/time_t.h>

struct AnnounceModel {
    int id;
    int torrent_id;
    std::string peer_ip;
    uint peer_port;
    time_t announce_time;
};