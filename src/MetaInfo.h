#ifndef METAINFO_H 

#include <string>
#include <vector>
#include <optional>

struct MetaInfo {
    // Dict info;
    std::string announce;
    std::vector<std::vector<std::string>> announce_list;
    std::optional<long> creation_date;
    std::optional<std::string> comment;
    std::optional<std::string> created_by;
    std::optional<std::string> encoding;
};

#endif