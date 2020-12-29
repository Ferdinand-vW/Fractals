#pragma once

#include <string>
#include <vector>
#include <optional>
#include <neither/neither.hpp>

#include "utils.h"
#include "maybe.h"

using namespace std;
using namespace neither;
using namespace std::string_literals;

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
    public:
    // InfoDict info;
        std::string announce;
        Maybe<std::vector<std::vector<std::string>>> announce_list;
        Maybe<int> creation_date;
        Maybe<std::string> comment;
        Maybe<std::string> created_by;
        Maybe<std::string> encoding;

        string to_string(int len = 100) {
            auto s_mi = "MetaInfo"s;
            auto s_ann = "{ announce: " + announce;
            
            auto m_ann_l_str = announce_list.map([](auto const l) -> string {
                auto comma_separated = [](const auto &ls) { return "[" + intercalate(", ",ls) + "]"; };
                std::vector<string> vs = map_vector<vector<string>,string>(l,comma_separated);
                return comma_separated(vs); 
                });
            auto s_ann_l = ", announce_list: "s + from_maybe(m_ann_l_str, "<empty>"s);
            auto int_to_string = [](const int& i) { return std::to_string(i); };
            auto s_cd    = ", creation_date: "s + from_maybe(creation_date.map(int_to_string), "<empty>"s);
            auto s_cmm   = ", comment: "s       + from_maybe(comment, "<empty>"s);
            auto s_cb    = ", created_by: "s    + from_maybe(created_by,"<empty>"s);
            auto s_enc   = ", encoding: "s      + from_maybe(encoding, "<empty>"s);
            auto s_end   = "}"s;

            auto endings = [len](const string &s) -> string {
                return s.substr(0,len) + "\n";
            };
            auto v = {s_mi,s_ann,s_ann_l,s_cd,s_cmm,s_cb,s_enc,s_end};
            return str_concat_vector(map_vector<string,string>(v,endings));
        }
};
