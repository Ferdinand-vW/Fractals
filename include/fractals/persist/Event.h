#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "fractals/persist/Models.h"

namespace fractals::persist
{

// Requests
struct AddTorrent
{
    AddTorrent() = default;

    AddTorrent(const std::string &infoHash, const std::string &name, const std::string &torrentFilePath,
               const std::string &writePath)
        : infoHash(infoHash), name(name), torrentFilePath(torrentFilePath), writePath(writePath)
    {
    }

    std::string infoHash;
    std::string name;
    std::string torrentFilePath;
    std::string writePath;
};

struct RemoveTorrent
{
    RemoveTorrent() = default;

    RemoveTorrent(const std::string &infoHash) : infoHash(infoHash){};

    std::string infoHash;
};

struct LoadTorrent
{
    LoadTorrent() = default;

    LoadTorrent(const std::string &infoHash) : infoHash(infoHash){};

    std::string infoHash;
};

struct LoadTorrents
{
};

struct AddPiece
{
    AddPiece() = default;

    AddPiece(const std::string &infoHash, uint32_t piece) : infoHash(infoHash), piece(piece)
    {
    }

    std::string infoHash;
    uint32_t piece;
};

struct RemovePieces
{
    RemovePieces() = default;

    RemovePieces(const std::string &infoHash) : infoHash(infoHash)
    {
    }

    std::string infoHash;
};

struct LoadPieces
{
    LoadPieces() = default;

    LoadPieces(const std::string &infoHash) : infoHash(infoHash)
    {
    }

    std::string infoHash;
};

struct AddAnnounce
{
    AddAnnounce() = default;

    AddAnnounce(const std::string &infoHash, const std::string &peerIp, uint16_t peerPort, time_t announceTime,
                int interval, std::optional<int> minInterval)
        : infoHash(infoHash)
    {
    }

    std::string infoHash;
    std::string peerIp;
    uint16_t peerPort;
    time_t announceTime;
    int interval;
    std::optional<int> minInterval;
};

struct RemoveAnnounces
{
    RemoveAnnounces() = default;

    RemoveAnnounces(const std::string &infoHash) : infoHash(infoHash)
    {
    }

    std::string infoHash;
};

struct LoadAnnounces
{
    LoadAnnounces() = default;

    LoadAnnounces(const std::string &infoHash) : infoHash(infoHash)
    {
    }

    std::string infoHash;
};

using PersistRequest = std::variant<AddTorrent, RemoveTorrent, LoadTorrent, LoadTorrents, AddPiece, RemovePieces,
                                    LoadPieces, AddAnnounce, RemoveAnnounces, LoadAnnounces>;

// Responses
struct Torrent
{
    std::string infoHash;
    TorrentModel result;
};

struct AllTorrents
{
    
    std::vector<TorrentModel> result;
};

struct Pieces
{
    std::string infoHash;
    std::vector<PieceModel> result;
};

struct Announces
{
    std::string infoHash;
    std::vector<AnnounceModel> result;
};

using PersistResponse = std::variant<Torrent, AllTorrents, Pieces, Announces>;
} // namespace fractals::persist