#pragma once

#include <string>

namespace fractals::persist {

    struct TorrentModel {
        int id;
        std::string name;
        std::string meta_info_path;
        std::string write_path;
    };

}