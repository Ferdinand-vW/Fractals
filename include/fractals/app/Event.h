#pragma once

#include <fractals/common/Tagged.h>
#include <fractals/persist/Models.h>
#include <fractals/torrent/TorrentMeta.h>
#include <string>
#include <variant>

namespace fractals::app
{
struct AddTorrent
{
    std::string filepath;
};

struct AddedTorrent
{
    common::InfoHash infoHash;
    persist::TorrentModel torrent;
    std::vector<persist::FileModel> files;
};

struct AddTorrentError
{
    std::string error;
};

struct ResumedTorrent
{
    common::InfoHash infoHash;
};

struct CompletedTorrent
{
    common::InfoHash infoHash;
};

struct RemoveTorrent
{
    common::InfoHash infoHash;
};

struct RemovedTorrent
{
    common::InfoHash infoHash;
};

struct StopTorrent
{
    common::InfoHash infoHash;
};

struct StoppedTorrent
{
    common::InfoHash infoHash;
};

struct StartTorrent
{
    common::InfoHash infoHash;
    persist::TorrentModel torrent;
    std::vector<persist::FileModel> files;
};

struct ResumeTorrent
{
    common::InfoHash infoHash;
};

struct Shutdown
{
};

struct ShutdownConfirmation
{
};

struct RequestStats
{
    std::vector<std::pair<uint64_t, common::InfoHash>> requested;
};

struct PeerStats
{
    uint64_t torrId;
    uint16_t knownPeerCount;
    uint16_t connectedPeersCount;
};

using RequestFromApp =
    std::variant<AddTorrent, RemoveTorrent, StopTorrent, StartTorrent, ResumeTorrent, Shutdown, RequestStats>;
using ResponseToApp = std::variant<AddedTorrent, AddTorrentError, ResumedTorrent, CompletedTorrent,
                                   RemovedTorrent, StoppedTorrent, ShutdownConfirmation, PeerStats>;
} // namespace fractals::app