#pragma once

#include "torrent/MetaInfo.h"
#include "torrent/PieceData.h"
#include <string>

struct FileData {
    FileInfo fi;
    int begin;
    int end;
};

class Torrent {
    public:
        std::string m_name;
        std::string m_dir;
        MetaInfo m_mi;
        std::vector<char> m_info_hash;
        std::vector<FileInfo> m_files;

        Torrent(MetaInfo &mi,std::string fileName);

        void write_data(PieceData && pd);
        static Torrent read_torrent(std::string fp);
        long long size_of_piece(int piece);

    private:
        void create_files(const std::vector<FileData> &fds);
        std::vector<FileData> divide_by_files(int piece,long long offset, long long len);
};