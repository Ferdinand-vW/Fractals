#include "fractals/common/Tagged.h"
#include "fractals/persist/Models.h"
#include "fractals/torrent/TorrentMeta.h"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace fractals::disk
{
struct InitTorrent
{
    persist::TorrentModel tm;
    std::vector<persist::FileModel> files;
};

struct Read
{
    std::string filepath;
};

struct ReadSuccess
{
    torrent::TorrentMeta tm;
    std::string readPath;
    std::string writePath;
};

struct ReadError
{
    std::string error;
};

struct WriteData
{
    common::InfoHash infoHash;
    uint32_t mPieceIndex;
    std::vector<char> mData;
    uint64_t offset;
};

struct DeleteTorrent
{
    common::InfoHash infoHash;
};

struct Shutdown
{
};

struct WriteSuccess
{
    common::InfoHash infoHash;
    uint32_t pieceIndex;
};

struct WriteError
{
    std::string error;
};

struct TorrentInitialized
{
    common::InfoHash infoHash;
};

using DiskRequest = std::variant<Read, InitTorrent, WriteData, DeleteTorrent, Shutdown>;
using DiskResponse =
    std::variant<ReadSuccess, ReadError, WriteSuccess, WriteError, TorrentInitialized>;
} // namespace fractals::disk