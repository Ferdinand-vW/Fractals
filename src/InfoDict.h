#ifndef INFODICT_H 

#include <string>
#include <vector>
#include <optional>
#include <map>

struct MetaInfoDict {
    // Dict info;
    std::int32_t pieceLength;
    std::string pieces;
    // if set to 1 then publish
    std::optional<std::int32_t> publish;
};

struct SingleFileInfo {
    std::string fileName;
    std::int32_t length;
    std::optional<std::string> md5sum;  
};

struct MultiFileInfo {
    std::string dirName;
    std::vector<SingleFileInfo> files;
};

struct SingleFileDict {
    SingleFileInfo file;
    MetaInfoDict info;
};

struct MultiFileDict {
    MultiFileInfo files;
    MetaInfoDict info;
};

#endif