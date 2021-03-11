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

    private:
        void create_files(const std::vector<FileData> &fds);
        std::vector<FileData> match_file_data(int piece,int offset, int len);
};