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
            TorrentMeta(const MetaInfo &mi,std::string fileName);

            /**
             * Returns name of torrent as described by the MetaInfo
             * @return name of torrent as std::string
             */
            std::string getName() const;

            /**
             * Name of outer directory if multi-file otherwise empty
             * @return possibly empty directory name as std::string
             */
            std::string getDirectory() const;

            /**
             * Returns all files contained in the torrent
             * @return vector of descriptions of files
             */
            std::vector<FileInfo> getFiles() const;

            /**
             * Returns original MetaInfo (parsed .torrent file)
             * @return MetaInfo object
             */
            MetaInfo getMetaInfo() const;

            /**
             * A SHA1 hash of Info dict in MetaInfo
             * @return SHA1 hash bytes represented as a vector of chars
             */
            std::vector<char> getInfoHash() const;

            /**
             * Compute size of torrent data
             */
             int64_t getSize() const;

        private:
            std::string m_name;
            std::string m_dir;
            std::vector<FileInfo> m_files;
            MetaInfo m_mi;
            std::vector<char> m_info_hash;
    };

}
