//
// Created by Ferdinand on 5/16/2022.
//

#pragma once

#include <string>

#include "fractals/torrent/MetaInfo.h"

namespace fractals::torrent {

    /**
    Abstraction of each file contained in the torrent data.
    */
    struct FileData {
        FileInfo fi;

        /**
        Offset in the torrent data.
        */
        int64_t begin;

        /**
        equals file size + begin
        */
        int64_t end;
        std::string full_path;
    };

}