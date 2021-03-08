#pragma once

#include "torrent/MetaInfo.h"
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
        std::vector<FileInfo> m_files;

        Torrent(MetaInfo &mi,std::string fileName);

        void write_data(int piece,int offset,std::vector<char> bytes);

    private:
        void create_files(const std::vector<FileData> &fd);
        std::vector<FileData> match_file_data(int piece,int offset, int len);
};