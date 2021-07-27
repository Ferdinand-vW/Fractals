#pragma once

#include <string>

namespace fractals::persist {

    /**
    ADT for announce peer model in database.
    Combines peers to specific announces.
    */
    struct AnnouncePeerModel {
        int id;
        int announce_id;
        std::string ip;
        int port;
    };

}