#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <fractals/common/maybe.h>
#include <fractals/common/utils.h>

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

    std::string to_string(int /* len = 100 */) const
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

    std::string to_string(int len = 100) const
    {
        auto sName = name.empty() ? "<empty>" : name;
        auto fileToString = [len](FileInfo f) { return f.to_string(len); };
        auto sFiles = common::mapVector<FileInfo, std::string>(files, fileToString);
        return "MF { directory: " + sName + "\n" + "[ " + common::intercalate("\n, ", sFiles) + "]";
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

    std::string to_string(int len = 100) const
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
    uint64_t pieceLength;
    std::vector<char> pieces;
    std::optional<int64_t> publish; // if set to 1 then publish
    std::variant<SingleFile, MultiFile> fileMode;

    std::string to_string(int len = 100) const
    {
        auto sId = "InfoDict"s;
        auto sPl = "{ piece length: "s + std::to_string(pieceLength);
        auto sPcs = ", pieces: <bytestring>"s;

        auto pbToString = [](const auto& a) { return std::to_string(a); };
        auto sPb = ", publish: "s + common::mapOpt(pbToString, publish).value_or("<empty>"s);

        auto sfToString = [len](const SingleFile& sf) { return sf.to_string(len); };
        auto mfToString = [len](const MultiFile& mf) { return mf.to_string(len); };
        auto sFm = ", file mode: "s +
                    common::eitherToVal<SingleFile, MultiFile, std::string>(fileMode, sfToString, mfToString);
        auto sEnd = "\n}\n"s;
        auto v = {sId, sPl, sPcs, sPb};
        auto toLine = [len](auto s) { return common::makeSizedLine(s, len); };
        return common::concat(common::mapVector<std::string, std::string>(v, toLine)) + sFm + sEnd;
    }

    uint64_t numberOfPieces() const
    {
        return pieces.size() / 20;
    }

    bool operator==(const InfoDict &info) const
    {
        return pieceLength == info.pieceLength && pieces == info.pieces && publish == info.publish &&
               fileMode == info.fileMode;
    }
};

/**
Structural representation of a Torrent file.
*/
struct MetaInfo
{
  public:
    std::string announce;
    std::vector<std::string> announceList;
    std::optional<int64_t> creationDate;
    std::optional<std::string> comment;
    std::optional<std::string> createdBy;
    std::optional<std::string> encoding;
    InfoDict info;

    std::string to_string(int len = 100)
    {
        auto sMi = "MetaInfo"s;
        auto sAnn = "{ announce: "s + announce;

        auto sAnnL = ", announceList: ["s + common::intercalate(", ", announceList) + "]";
        auto cdToString = [](const auto& a) { return std::to_string(a); };
        auto sCd = ", creationDate: "s + common::mapOpt(cdToString, creationDate).value_or("<empty>"s);
        auto sCmm = ", comment: "s + comment.value_or("<empty>"s);
        auto sCb = ", createdBy: "s + createdBy.value_or("<empty>"s);
        auto sEnc = ", encoding: "s + encoding.value_or("<empty>"s);
        auto sInfo = ", info: "s + info.to_string(len);
        auto sEnd = "\n}\n"s;

        auto v = {sMi, sAnn, sAnnL, sCd, sCmm, sCb, sEnc};
        // shorten string to max size and add new lines then concat
        auto toLine = [len](const auto& s) { return common::makeSizedLine(s, len); };
        return common::concat(common::mapVector<std::string, std::string>(v, toLine)) + sInfo + sEnd;
    }

    bool operator==(const MetaInfo &metaInfo) const
    {
        bool b = announce == metaInfo.announce && announceList == metaInfo.announceList &&
                 creationDate == metaInfo.creationDate && comment == metaInfo.comment &&
                 createdBy == metaInfo.createdBy && encoding == metaInfo.encoding && info == metaInfo.info;
        return b;
    }
};

} // namespace fractals::torrent