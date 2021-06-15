#pragma once

#include "torrent/MetaInfo.h"
#include "torrent/PieceData.h"
#include "common/logger.h"
#include <boost/log/sources/logger.hpp>
#include <string>

struct FileData {
    FileInfo fi;
    long long begin;
    long long end;
    std::string full_path;
};

class Torrent {
    public:
        std::string m_name;
        std::string m_dir;
        MetaInfo m_mi;
        std::vector<char> m_info_hash;
        std::vector<FileInfo> m_files;
        boost::log::sources::logger_mt &m_lg;

        Torrent(MetaInfo &mi,std::string fileName,std::set<int> pieces);

        void write_data(PieceData && pd);
        static Torrent read_torrent(std::string fp);
        long long size_of_piece(int piece);
        long long size_of_pieces(std::set<int> pieces);
        void add_piece(int p);
        void add_piece(std::set<int> p);
        std::set<int> get_pieces();

    private:
        long long cumulative_size_of_pieces(int piece);
        void create_files(std::vector<FileData> &fds);
        std::vector<FileData> divide_over_files(int piece);
        std::set<int> m_pieces;
};