#pragma once

#include <string>

struct AnnouncePeerModel {
    int id;
    int announce_id;
    std::string ip;
    int port;
};