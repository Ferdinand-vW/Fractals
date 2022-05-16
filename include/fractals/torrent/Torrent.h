#pragma once

#include <string>

#include <boost/log/sources/logger.hpp>

#include "fractals/torrent/MetaInfo.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::torrent {

    struct PieceData;

    /**
    This class represents both the MetaInfo file and the (partially) downloaded torrent data.
    Responsibilities are:
    1) Write new piece data to the torrent data files at the correct offsets.
    2) Allow querying for piece meta information (size, offset, etc)
    3) Parse torrent file to MetaInfo/Torrent
    */
    class Torrent {

        public:


            boost::log::sources::logger_mt &m_lg;
            std::mutex m_mutex;

            Torrent(MetaInfo &mi,std::string fileName,std::set<int> pieces);

            void write_data(PieceData & pd);
            static Either<std::string,std::shared_ptr<Torrent>> read_torrent(std::string fp);
            int64_t size_of_piece(int piece);
            int64_t size_of_pieces(std::set<int> pieces);
            void add_piece(int p);
            void add_piece(std::set<int> p);
            std::set<int> get_pieces();

        private:
            TorrentMeta m_tm;

            /**
            Add up size of each piece up to and including @piece.
            Can be used to calculate offset of piece by cumulative_size_of_pieces(piece - 1)
            */
            int64_t cumulative_size_of_pieces(int piece);
            
            void create_files(std::vector<FileData> &fds);

            /**
            Generate files for which the piece must write data to
            */
            std::vector<FileData> divide_over_files(int piece);
            std::set<int> m_pieces;
    };

}