//
// Created by Ferdinand on 5/14/2022.
//

#pragma once

#include <string>
#include <vector>

#include "fractals/torrent/MetaInfo.h"

namespace fractals::torrent {

    struct TorrentMeta {
        public:
            /**
             * Returns name of torrent as described by the MetaInfo
             * @return name of torrent as std::string
             */
            std::string getName();

            /**
             * Name of outer directory if multi-file otherwise empty
             * @return possibly empty directory name as std::string
             */
            std::string getDirectory();

            /**
             * Returns all files contained in the torrent
             * @return vector of descriptions of files
             */
            std::vector<FileInfo> getFiles();

            /**
             * Returns original MetaInfo (parsed .torrent file)
             * @return MetaInfo object
             */
            MetaInfo getTorrentFile();

            /**
             * A SHA1 hash of Info dict in MetaInfo
             * @return SHA1 hash bytes represented as a vector of chars
             */
            std::vector<char> getInfoHash();

        private:
            std::string m_name;
            std::string m_dir;
            std::vector<FileInfo> m_files;
            MetaInfo m_mi;
            std::vector<char> m_info_hash;
    };

}
