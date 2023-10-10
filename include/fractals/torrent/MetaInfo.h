#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "fractals/common/maybe.h"
#include "fractals/common/utils.h"

using namespace neither;
using namespace std::string_literals;

namespace fractals::torrent
{

struct FileInfo
{
  public:
    uint64_t length;
    std::vector<char> md5sum;
    std::vector<std::string> path;

    std::string to_string(int /* len = 100 */)
    {
        const auto md5sumStr = md5sum.empty() ? "<empty>" : std::string(md5sum.begin(), md5sum.end());
        return "{ length =  "s + std::to_string(length) + ", path = [" + common::intercalate(",", path) +
               "], md5sum = " + md5sumStr + "}";
    }

    bool operator==(const FileInfo &file) const
    {
        return length == file.length && md5sum == file.md5sum && path == file.path;
    }
};

struct MultiFile
{
  public:
    std::string name; // directory name
    std::vector<FileInfo> files;

    std::string to_string(int len = 100)
    {
        auto s_name = name.empty() ? "<empty>" : name;
        auto file_to_string = [len](FileInfo f) { return f.to_string(len); };
        auto s_files = common::map_vector<FileInfo, std::string>(files, file_to_string);
        return "MF { directory: " + s_name + "\n" + "[ " + common::intercalate("\n, ", s_files) + "]";
    }

    bool operator==(const MultiFile &file) const
    {
        return name == file.name && files == file.files;
    }
};

struct SingleFile
{
  public:
    std::string name; // file name
    uint64_t length;
    std::vector<char> md5sum;

    std::string to_string(int len = 100)
    {
        const auto nameStr = name.empty() ? "<empty>" : name;
        const auto md5sumStr = md5sum.empty() ? "<empty>" : std::string(md5sum.begin(), md5sum.end());
        return "SF { name: " + nameStr + ", length =  "s + std::to_string(length) + "], md5sum = " + md5sumStr + "}";
    }

    bool operator==(const SingleFile &file) const
    {
        return name == file.name && length == file.length && md5sum == file.md5sum;
    }
};

struct InfoDict
{
  public:
    uint64_t piece_length;
    std::vector<char> pieces;
    std::optional<int64_t> publish; // if set to 1 then publish
    std::variant<SingleFile, MultiFile> file_mode;

    std::string to_string(int len = 100)
    {
        auto s_id = "InfoDict"s;
        auto s_pl = "{ piece length: "s + std::to_string(piece_length);
        auto s_pcs = ", pieces: <bytestring>"s;

        auto pb_to_string = [](auto a) { return std::to_string(a); };
        auto s_pb = ", publish: "s + common::map_opt(pb_to_string, publish).value_or("<empty>"s);

        auto sf_to_string = [len](SingleFile sf) { return sf.to_string(len); };
        auto mf_to_string = [len](MultiFile mf) { return mf.to_string(len); };
        auto s_fm = ", file mode: "s +
                    common::either_to_val<SingleFile, MultiFile, std::string>(file_mode, sf_to_string, mf_to_string);
        auto s_end = "\n}\n"s;
        auto v = {s_id, s_pl, s_pcs, s_pb};
        auto to_line = [len](auto s) { return common::make_sized_line(s, len); };
        return common::concat(common::map_vector<std::string, std::string>(v, to_line)) + s_fm + s_end;
    }

    uint64_t number_of_pieces() const
    {
        return pieces.size() / 20;
    }

    bool operator==(const InfoDict &info) const
    {
        return piece_length == info.piece_length && pieces == info.pieces && publish == info.publish &&
               file_mode == info.file_mode;
    }
};

/**
Structural representation of a Torrent file.
*/
struct MetaInfo
{
  public:
    std::string announce;
    std::vector<std::string> announce_list;
    std::optional<int64_t> creation_date;
    std::optional<std::string> comment;
    std::optional<std::string> created_by;
    std::optional<std::string> encoding;
    InfoDict info;

    std::string to_string(int len = 100)
    {
        auto s_mi = "MetaInfo"s;
        auto s_ann = "{ announce: "s + announce;

        auto s_ann_l = ", announce_list: ["s + common::intercalate(", ", announce_list) + "]";
        auto int_to_string = [](const auto &i) { return std::to_string(i); };
        auto s_cd = ", creation_date: "s + common::map_opt(int_to_string, creation_date).value_or("<empty>"s);
        auto s_cmm = ", comment: "s + comment.value_or("<empty>"s);
        auto s_cb = ", created_by: "s + created_by.value_or("<empty>"s);
        auto s_enc = ", encoding: "s + encoding.value_or("<empty>"s);
        auto s_info = ", info: "s + info.to_string(len);
        auto s_end = "\n}\n"s;

        auto v = {s_mi, s_ann, s_ann_l, s_cd, s_cmm, s_cb, s_enc};
        // shorten string to max size and add new lines then concat
        auto to_line = [len](auto s) { return common::make_sized_line(s, len); };
        return common::concat(common::map_vector<std::string, std::string>(v, to_line)) + s_info + s_end;
    }

    bool operator==(const MetaInfo &metaInfo) const
    {
        bool b = announce == metaInfo.announce && announce_list == metaInfo.announce_list &&
                 creation_date == metaInfo.creation_date && comment == metaInfo.comment &&
                 created_by == metaInfo.created_by && encoding == metaInfo.encoding && info == metaInfo.info;
        return b;
    }
};

} // namespace fractals::torrent