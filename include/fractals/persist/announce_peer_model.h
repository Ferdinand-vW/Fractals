#pragma once

#include <string>

namespace fractals::persist {

    struct AnnouncePeerModel {
        int id;
        int announce_id;
        std::string ip;
        int port;
    };

}