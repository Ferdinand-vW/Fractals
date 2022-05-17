#pragma once

#include <string>

#include <boost/log/sources/logger.hpp>

#include "fractals/torrent/MetaInfo.h"
#include "fractals/torrent/TorrentMeta.h"
#include "fractals/torrent/TorrentIO.h"

namespace fractals::torrent {

    struct Piece;

    /**
    This class represents both the MetaInfo file and the (partially) downloaded torrent data.
    Responsibilities are:
    1) Write new piece data to the torrent data files at the correct offsets.
    2) Allow querying for piece meta information (size, offset, etc)
    3) Parse torrent file to MetaInfo/Torrent
    */
    class Torrent {

        public:

            Torrent(TorrentMeta &&tm,TorrentIO &tio, std::set<int> pieces);

            const TorrentMeta& getMeta() const;
            TorrentIO& getIO() const;

            void add_piece(int p);
            void add_piece(std::set<int> p);
            std::set<int> get_pieces();

            std::string getName() const;
            const MetaInfo& getMetaInfo() const;
            void writePiece(Piece &&p);

        private:
            TorrentMeta m_tm;
            TorrentIO &m_io;
            std::set<int> m_pieces; // in-memory representation of available pieces
    };

}