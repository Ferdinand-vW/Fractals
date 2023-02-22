#pragma once

#include <functional>
#include <string>
#include <vector>
#include <optional>

#include <neither/neither.hpp>

#include "fractals/common/utils.h"
#include "fractals/common/maybe.h"

using namespace neither;
using namespace std::string_literals;

namespace fractals::torrent {

    struct FileInfo {
        public:
            int64_t length;
            Maybe<std::vector<char>> md5sum;
            std::vector<std::string> path;


            std::string to_string(int /* len = 100 */) {
                Maybe<std::string> md5sum_s = md5sum.map([](const auto &v) { return std::string(v.begin(),v.end()); });
                return "{ length =  "s + std::to_string(length) + 
                       ", path = [" + common::intercalate(",",path) + 
                       "], md5sum = " + common::from_maybe(md5sum_s,"<empty>"s) + "}";
            }

            bool operator==(const FileInfo& file) const
            {
                return length == file.length
                    && md5sum == file.md5sum
                    && path == file.path;
            }
    };

    struct MultiFile {
        public:
            Maybe<std::string> name; // directory name
            std::vector<FileInfo> files;

            std::string to_string(int len = 100) {
                auto s_name = common::from_maybe(name,"<empty>"s);
                auto file_to_string = [len](FileInfo f) { return f.to_string(len); };
                auto s_files = common::map_vector<FileInfo, std::string>(files,file_to_string);
                return "MF { directory: " + s_name + "\n" + "[ " + common::intercalate("\n, ",s_files) + "]";
            }

            bool operator==(const MultiFile& file) const
            {
                return name == file.name
                    && files == file.files;
            }
    };

    struct SingleFile {
        public:
            Maybe<std::string> name; // file name
            int64_t length;
            Maybe<std::vector<char>> md5sum;

            std::string to_string(int len = 100) {
                Maybe<std::string> md5sum_s = md5sum.map([](const auto &v) { return std::string(v.begin(),v.end()); });
                std::string s ("SF { name: " + common::from_maybe(name,"<empty>"s) + 
                          ", length: " + std::to_string(length) + ", md5sum: " + 
                          common::from_maybe(md5sum_s,"<empty>"s));
                return s;
            }

            bool operator==(const SingleFile& file) const
            {
                return name == file.name
                    && length == file.length
                    && md5sum == file.md5sum;
            }
    };

    struct InfoDict {
        public:
            int64_t piece_length;
            std::vector<char> pieces;
            Maybe<int64_t> publish; // if set to 1 then publish
            Either<SingleFile,MultiFile> file_mode;

            std::string to_string(int len = 100) {
                auto s_id  = "InfoDict"s;
                auto s_pl  = "{ piece length: "s + std::to_string(piece_length);
                auto s_pcs = ", pieces: <bytestring>"s;

                auto pb_to_string = [](auto a) { return std::to_string(a); };
                auto s_pb  = ", publish: "s + common::maybe_to_val<int64_t, std::string>(publish,pb_to_string,"<empty>"s);
                
                auto sf_to_string = [len](SingleFile sf) { return sf.to_string(len); };
                auto mf_to_string = [len](MultiFile mf) { return mf.to_string(len); };
                auto s_fm  = ", file mode: "s + common::either_to_val<SingleFile,MultiFile, std::string>(file_mode,sf_to_string,mf_to_string);
                auto s_end = "\n}\n"s;
                auto v = {s_id,s_pl,s_pcs,s_pb};
                auto to_line = [len](auto s) { return common::make_sized_line(s,len); };
                return common::concat(common::map_vector<std::string, std::string>(v,to_line)) + s_fm + s_end;
            }

            int64_t number_of_pieces() const {
                return pieces.size() / 20;
            }

            bool operator==(const InfoDict& info) const
            {
                return piece_length == info.piece_length
                    && pieces == info.pieces
                    && publish == info.publish
                    && file_mode == info.file_mode;
            }
    };

    /**
    Structural representation of a Torrent file.
    */
    struct MetaInfo {
        public:
            std::string announce;
            Maybe<std::vector<std::vector<std::string>>> announce_list;
            Maybe<int64_t> creation_date;
            Maybe<std::string> comment;
            Maybe<std::string> created_by;
            Maybe<std::string> encoding;
            InfoDict info;

            std::string to_string(int len = 100) {
                auto s_mi = "MetaInfo"s;
                auto s_ann = "{ announce: "s + announce;
                
                auto m_ann_l_str = announce_list.map([](auto const l) -> std::string {
                    auto comma_separated = [](const auto &ls) { return "[" + common::intercalate(", ",ls) + "]"; };
                    std::vector<std::string> vs = common::map_vector<std::vector<std::string>, std::string>(l,comma_separated);
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
                return common::concat(common::map_vector<std::string, std::string>(v,to_line)) + s_info + s_end;
            }

            bool operator==(const MetaInfo& metaInfo) const
            {
                bool b= announce == metaInfo.announce
                    && announce_list == metaInfo.announce_list
                    && creation_date == metaInfo.creation_date
                    && comment == metaInfo.comment
                    && created_by == metaInfo.created_by
                    && encoding == metaInfo.encoding
                    && info == metaInfo.info;
                return b;
            }
    };

}