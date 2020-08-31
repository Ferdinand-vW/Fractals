#ifndef METAINFO_H 

#include <string>
#include <vector>
#include <optional>

struct FileInfo {
    std::string fileName;
    std::int32_t length;
    std::optional<std::string> md5sum;  
};

struct InfoDict {
    std::int32_t pieceLength;
    std::string pieces;
    // if set to 1 then publish
    std::optional<std::int32_t> publish;
    std::string directory;
    std::vector<FileInfo> files;
};



struct MetaInfo {
    InfoDict info;
    std::string announce;
    std::vector<std::vector<std::string>> announce_list;
    std::optional<long> creation_date;
    std::optional<std::string> comment;
    std::optional<std::string> created_by;
    std::optional<std::string> encoding;
};




#endif