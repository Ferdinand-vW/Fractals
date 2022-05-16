//
// Created by Ferdinand on 5/14/2022.
//

#include "fractals/torrent/TorrentMeta.h"

namespace fractals::torrent {

    std::string TorrentMeta::getName() {
        return m_name;
    }

    std::string TorrentMeta::getDirectory() {
        return m_dir;
    }

    std::vector<FileInfo> TorrentMeta::getFiles() {
        return m_files;
    }



}