#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "fractals/common/Tagged.h"
#include "fractals/persist/Models.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::persist
{

// Requests
struct AddTorrent
{
    AddTorrent() = default;

    AddTorrent(const torrent::TorrentMeta &tm, const std::string &torrentFilePath,
               const std::string &writePath)
        : tm(tm), torrentFilePath(torrentFilePath), writePath(writePath)
    {
    }

    torrent::TorrentMeta tm;
    std::string torrentFilePath;
    std::string writePath;
};

struct RemoveTorrent
{
    RemoveTorrent() = default;

    RemoveTorrent(const common::InfoHash &infoHash) : infoHash(infoHash){};

    common::InfoHash infoHash;
};

struct LoadTorrents
{
};

struct AddTrackers
{
    common::InfoHash infoHash;
    std::vector<std::string> trackers;
};

struct LoadTrackers
{
    common::InfoHash infoHash;
};

struct PieceComplete
{
    PieceComplete() = default;

    PieceComplete(const common::InfoHash &infoHash, uint32_t piece)
        : infoHash(infoHash), piece(piece)
    {
    }

    common::InfoHash infoHash;
    uint32_t piece;
};

struct RemovePieces
{
    RemovePieces() = default;

    RemovePieces(const common::InfoHash &infoHash) : infoHash(infoHash)
    {
    }

    common::InfoHash infoHash;
};

struct LoadPieces
{
    LoadPieces() = default;

    LoadPieces(const common::InfoHash &infoHash) : infoHash(infoHash)
    {
    }

    common::InfoHash infoHash;
};

struct AddAnnounce
{
    AddAnnounce() = default;

    AddAnnounce(const common::InfoHash &infoHash, const std::string &peerIp, uint16_t peerPort,
                time_t announceTime, int interval, std::optional<int> minInterval)
        : infoHash(infoHash)
    {
    }

    common::InfoHash infoHash;
    std::string peerIp;
    uint16_t peerPort;
    time_t announceTime;
    int interval;
    std::optional<int> minInterval;
};

struct RemoveAnnounces
{
    RemoveAnnounces() = default;

    RemoveAnnounces(const common::InfoHash &infoHash) : infoHash(infoHash)
    {
    }

    common::InfoHash infoHash;
};

struct LoadAnnounces
{
    LoadAnnounces() = default;

    LoadAnnounces(const common::InfoHash &infoHash) : infoHash(infoHash)
    {
    }

    common::InfoHash infoHash;
};

struct RequestStats
{
    std::vector<std::pair<uint64_t, common::InfoHash>> requested;
};

struct Shutdown
{
};

using PersistRequest =
    std::variant<AddTorrent, RemoveTorrent, AddTrackers, LoadTrackers, PieceComplete, RemovePieces,
                 LoadPieces, AddAnnounce, RemoveAnnounces, LoadAnnounces, Shutdown>;

using AppPersistRequest = std::variant<LoadTorrents, RequestStats>;

// Responses
struct AddedTorrent
{
    persist::TorrentModel torrent;
    std::vector<FileModel> files;
};

struct TorrentExists
{
    common::InfoHash infoHash;
};

struct AllTorrents
{
    std::vector<std::pair<TorrentModel, std::vector<FileModel>>> result;
};

struct Trackers
{
    common::InfoHash infoHash;
    std::vector<TrackerModel> trackers;
};

struct Pieces
{
    common::InfoHash infoHash;
    std::vector<PieceModel> result;
};

struct Announces
{
    common::InfoHash infoHash;
    std::vector<AnnounceModel> result;
};

struct TorrentStats
{
    uint64_t torrId{0};
    common::InfoHash infoHash;
    uint64_t downloaded{0};
    uint64_t uploaded{0};
    uint64_t connectedSeeders{0};
    uint64_t totalSeeders{0};
    uint64_t connectedLeechers{0};
    uint64_t totalLeechers{0};
};

using PersistResponse = std::variant<AddedTorrent, TorrentExists, Trackers, Pieces, Announces>;

using AppPersistResponse = std::variant<AllTorrents, TorrentStats>;
} // namespace fractals::persist