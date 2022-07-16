//
// Created by Ferdinand on 5/14/2022.
//

#pragma once

#include <neither/neither.hpp>
#include <boost/log/sources/logger.hpp>

#include "fractals/persist/storage.h"

namespace fractals::torrent {

    struct TorrentMeta;
    struct Piece;
    class File;

    class TorrentIO {
        public:
            TorrentIO(persist::Storage &storage,boost::log::sources::logger_mt &lg)
                    : m_storage(storage)
                    , m_lg(lg) {};

            void writePieceToDisk(const TorrentMeta &tm, Piece &&pd);
            void writePieceToDb(const TorrentMeta &tm,int piece);
            void createFiles(std::string dir, std::vector<File> &fds);
            static neither::Either<std::string,TorrentMeta> readTorrentFile(std::string fp);

        private:
            persist::Storage &m_storage;
            std::mutex m_mutex;
            boost::log::sources::logger_mt &m_lg;


    };

}