#pragma once

#include <functional>
#include <string>
#include <vector>
#include <optional>

#include <neither/neither.hpp>

#include "fractals/common/utils.h"
#include "fractals/common/maybe.h"

using namespace std;
using namespace neither;
using namespace std::string_literals;

namespace fractals::torrent {

    struct FileInfo {
        public:
            long long length;
            Maybe<std::vector<char>> md5sum;
            std::vector<string> path;


            string to_string(int /* len = 100 */) {
                Maybe<string> md5sum_s = md5sum.map([](const auto &v) { return string(v.begin(),v.end()); });
                return "{ length =  "s + std::to_string(length) + 
                       ", path = [" + common::intercalate(",",path) + 
                       "], md5sum = " + common::from_maybe(md5sum_s,"<empty>"s) + "}";
            }
    };

    struct MultiFile {
        public:
            Maybe<std::string> name; // directory name
            std::vector<FileInfo> files;

            string to_string(int len = 100) {
                auto s_name = common::from_maybe(name,"<empty>"s);
                auto file_to_string = [len](FileInfo f) { return f.to_string(len); };
                auto s_files = common::map_vector<FileInfo,string>(files,file_to_string);
                return "MF { directory: " + s_name + "\n" + "[ " + common::intercalate("\n, ",s_files) + "]";
            }
    };

    struct SingleFile {
        public:
            Maybe<std::string> name; // file name
            long long length;
            Maybe<std::vector<char>> md5sum;

            string to_string(int len = 100) {
                Maybe<string> md5sum_s = md5sum.map([](const auto &v) { return string(v.begin(),v.end()); });
                string s ("SF { name: " + common::from_maybe(name,"<empty>"s) + 
                          ", length: " + std::to_string(length) + ", md5sum: " + 
                          common::from_maybe(md5sum_s,"<empty>"s));
                return s;
            }
    };

    struct InfoDict {
        public:
            long long piece_length;
            std::vector<char> pieces;
            Maybe<long long> publish; // if set to 1 then publish
            Either<SingleFile,MultiFile> file_mode;

            string to_string(int len = 100) {
                auto s_id  = "InfoDict"s;
                auto s_pl  = "{ piece length: "s + std::to_string(piece_length);
                auto s_pcs = ", pieces: <bytestring>"s;

                auto pb_to_string = [](auto a) { return std::to_string(a); };
                auto s_pb  = ", publish: "s + common::maybe_to_val<long long,string>(publish,pb_to_string,"<empty>"s);
                
                auto sf_to_string = [len](SingleFile sf) { return sf.to_string(len); };
                auto mf_to_string = [len](MultiFile mf) { return mf.to_string(len); };
                auto s_fm  = ", file mode: "s + common::either_to_val<SingleFile,MultiFile,string>(file_mode,sf_to_string,mf_to_string);
                auto s_end = "\n}\n"s;
                auto v = {s_id,s_pl,s_pcs,s_pb};
                auto to_line = [len](auto s) { return common::make_sized_line(s,len); };
                return common::str_concat_vector(common::map_vector<string,string>(v,to_line)) + s_fm + s_end;
            }

            long long number_of_pieces() {
                return pieces.size() / 20;
            }
    };

    struct MetaInfo {
        public:
            std::string announce;
            Maybe<std::vector<std::vector<std::string>>> announce_list;
            Maybe<long long> creation_date;
            Maybe<std::string> comment;
            Maybe<std::string> created_by;
            Maybe<std::string> encoding;
            InfoDict info;

            string to_string(int len = 100) {
                auto s_mi = "MetaInfo"s;
                auto s_ann = "{ announce: "s + announce;
                
                auto m_ann_l_str = announce_list.map([](auto const l) -> string {
                    auto comma_separated = [](const auto &ls) { return "[" + common::intercalate(", ",ls) + "]"; };
                    std::vector<string> vs = common::map_vector<vector<string>,string>(l,comma_separated);
                    return comma_separated(vs); 
                    });
                auto s_ann_l = ", announce_list: "s + common::from_maybe(m_ann_l_str, "<empty>"s);
                auto int_to_string = [](const auto& i) { return std::to_string(i); };
                auto s_cd    = ", creation_date: "s + common::from_maybe(creation_date.map(int_to_string), "<empty>"s);
                auto s_cmm   = ", comment: "s       + common::from_maybe(comment, "<empty>"s);
                auto s_cb    = ", created_by: "s    + common::from_maybe(created_by,"<empty>"s);
                auto s_enc   = ", encoding: "s      + common::from_maybe(encoding, "<empty>"s);
                auto s_info  = ", info: "s          + info.to_string(len);
                auto s_end   = "\n}\n"s;

                auto v = {s_mi,s_ann,s_ann_l,s_cd,s_cmm,s_cb,s_enc};
                // shorten string to max size and add new lines then concat
                auto to_line = [len](auto s) { return common::make_sized_line(s,len); };
                return common::str_concat_vector(common::map_vector<string,string>(v,to_line)) + s_info + s_end;
            }
    };

}