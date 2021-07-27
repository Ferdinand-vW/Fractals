#pragma once

#include <string>

namespace fractals::persist {

    /**
    ADT for torrent model in database
    */
    struct TorrentModel {
        int id;
        std::string name;
        std::string meta_info_path;
        std::string write_path;
    };

}