#include <filesystem>
#include <mutex>
#include <numeric>
#include <optional>
#include <spdlog/spdlog.h>
#include <string_view>

#include <fractals/common/Tagged.h>
#include <fractals/common/utils.h>
#include <fractals/persist/Event.h>
#include <fractals/persist/Models.h>
#include <fractals/persist/PersistClient.h>

namespace fractals::persist
{

using namespace sqlite_orm;

void PersistClient::openConnection(const std::string &connString)
{
    if (!dbConn || !dbConn->is_opened())
    {
        // connect to user given db and create tables if not already present
        auto db = makeDatabase(connString);
        // update pointer to point to new database connection
        dbConn = std::make_unique<Schema>(db);
    }

    dbConn->sync_schema();
}

std::variant<AddedTorrent, TorrentExists> PersistClient::addTorrent(const AddTorrent &at)
{
    const auto &infoHash = at.tm.getInfoHash();
    if (torrentIds.contains(at.tm.getInfoHash()))
    {
        return TorrentExists{infoHash};
    }

    const auto &mi = at.tm.getMetaInfo();
    TorrentModel torrent{-1,
                         infoHash.toString(),
                         at.tm.getName(),
                         at.tm.getDirectory(),
                         at.torrentFilePath,
                         at.writePath,
                         at.tm.getSize(),
                         mi.info.pieces.size(),
                         mi.info.pieceLength,
                         mi.creationDate,
                         mi.comment,
                         mi.createdBy,
                         mi.encoding,
                         mi.info.publish};
    const auto torrId = dbConn->insert(torrent);
    torrentIds.emplace(infoHash, torrId);

    std::vector<FileModel> files;
    for (const auto &fi : at.tm.getFiles())
    {
        const auto fullName = std::filesystem::path(common::intercalate("/", fi.path));
        FileModel file{-1,
                       torrId,
                       fullName.filename(),
                       fullName.parent_path().string(),
                       fi.length,
                       std::string(fi.md5sum.begin(), fi.md5sum.end())};
        dbConn->insert(file);
        files.emplace_back(file);
    }

    const auto &infoDict = at.tm.getMetaInfo().info;
    std::string_view vw(infoDict.pieces.begin(), infoDict.pieces.end());
    uint32_t numPieces = infoDict.numberOfPieces();
    uint32_t pieceIndex{0};
    static constexpr uint64_t hashLength{20};
    for (; pieceIndex < numPieces - 1; ++pieceIndex)
    {
        const auto hashView = vw.substr(pieceIndex * hashLength, hashLength);
        dbConn->insert(PieceModel{-1, torrId, pieceIndex, infoDict.pieceLength,
                                  std::vector<char>(hashView.begin(), hashView.end()), false});
    }

    const auto cumulativeSize = infoDict.pieceLength * (infoDict.numberOfPieces() - 1);
    // last piece usually has size different from pieceLength
    const auto lastPieceSize = at.tm.getSize() - cumulativeSize;

    const auto hashView = vw.substr(pieceIndex * hashLength, hashLength);
    dbConn->insert(PieceModel{-1, torrId, pieceIndex, lastPieceSize,
                              std::vector<char>(hashView.begin(), hashView.end()), false});

    TrackerModel mainTracker{-1, torrId, at.tm.getMetaInfo().announce};
    dbConn->insert(mainTracker);
    for (const auto &annUrl : at.tm.getMetaInfo().announceList)
    {
        dbConn->insert(TrackerModel{-1, torrId, annUrl});
    }

    loadTorrentStats(infoHash);

    torrent.id = torrId;
    return AddedTorrent{torrent, files};
}

std::optional<TorrentModel> PersistClient::loadTorrent(const common::InfoHash &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        auto tms = dbConn->get_all<TorrentModel>(where(c(&TorrentModel::id) == torrId->second));

        if (tms.begin() != tms.end())
        {
            loadTorrentStats(infoHash);
            return tms.front();
        }
    }

    return std::nullopt;
}

std::vector<std::pair<TorrentModel, std::vector<FileModel>>> PersistClient::loadTorrents()
{
    const auto torrs = dbConn->get_all<TorrentModel>();

    std::vector<std::pair<TorrentModel, std::vector<FileModel>>> result;
    for (const auto &torr : torrs)
    {
        torrentIds.emplace(common::InfoHash{torr.infoHash}, torr.id);
        const auto files = dbConn->get_all<FileModel>(where(c(&FileModel::torrentId) == torr.id));

        result.emplace_back(std::make_pair(torr, files));
    }

    return result;
}

void PersistClient::deleteTorrent(const common::InfoHash &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->remove_all<PieceModel>(where(c(&PieceModel::torrentId) == torrId->second));
        dbConn->remove_all<AnnounceModel>(where(c(&AnnounceModel::torrentId) == torrId->second));
        dbConn->remove_all<TrackerModel>(where(c(&TrackerModel::torrentId) == torrId->second));
        dbConn->remove_all<FileModel>(where(c(&FileModel::torrentId) == torrId->second));
        dbConn->remove<TorrentModel>(torrId->second);
        torrentIds.erase(infoHash);
        inMemoryStats.erase(infoHash);
    }
}

void PersistClient::addTrackers(const AddTrackers &req)
{
    auto torrId = torrentIds.find(req.infoHash);
    if (torrId != torrentIds.end())
    {
        for (const auto &tracker : req.trackers)
        {
            dbConn->insert(TrackerModel{-1, torrId->second, tracker});
        }
    }
}

std::vector<TrackerModel> PersistClient::loadTrackers(const common::InfoHash &ih)
{
    auto torrId = torrentIds.find(ih);
    if (torrId != torrentIds.end())
    {
        return dbConn->get_all<TrackerModel>(where(c(&TrackerModel::torrentId) == torrId->second));
    }

    return {};
}

void PersistClient::addPiece(const PieceComplete &pm)
{
    auto torrId = torrentIds.find(pm.infoHash);
    if (torrId != torrentIds.end())
    {
        auto pieceResults = dbConn->get_all<PieceModel>(where(
            c(&PieceModel::piece) == pm.piece && c(&PieceModel::torrentId) == torrId->second));

        assert(pieceResults.size() == 1);
        auto piece = pieceResults.front();
        piece.complete = true;
        dbConn->update(piece);

        inMemoryStats[torrId->first].downloaded += piece.size;
    }
}

std::vector<PieceModel> PersistClient::loadPieces(const common::InfoHash &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        return dbConn->get_all<PieceModel>(where(c(&PieceModel::torrentId) == torrId->second));
    }

    return {};
}

void PersistClient::deletePieces(const common::InfoHash &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->remove_all<PieceModel>(where(c(&PieceModel::torrentId) == torrId->second));

        assert(false); // update inmemory?
    }
}

void PersistClient::addAnnounce(const AddAnnounce &ann)
{
    auto torrId = torrentIds.find(ann.infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->insert(AnnounceModel{-1, torrId->second, ann.peerIp, ann.peerPort, ann.announceTime,
                                     ann.interval, ann.minInterval, false});
    }
}

void PersistClient::deleteAnnounce(const common::InfoHash &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->remove_all<AnnounceModel>(where(c(&AnnounceModel::torrentId) == torrId->second));
        assert(false); // update inmemory?
    }
}

std::vector<AnnounceModel> PersistClient::loadAnnounces(const common::InfoHash &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        return dbConn->get_all<AnnounceModel>(
            where(c(&AnnounceModel::torrentId) == torrId->second));
    }

    return {};
}

TorrentStats PersistClient::loadTorrentStats(const common::InfoHash &infoHash)
{
    auto torrStats = inMemoryStats.find(infoHash);
    if (torrStats != inMemoryStats.end())
    {
        return torrStats->second;
    }

    auto pieces = loadPieces(infoHash);

    TorrentStats stats;
    stats.infoHash = infoHash;
    stats.downloaded = std::accumulate(pieces.begin(), pieces.end(), 0,
                                       [](const auto &acc, const auto &p2)
                                       {
                                           return acc + (p2.complete ? p2.size : 0);
                                       });
    inMemoryStats.emplace(infoHash, stats);

    return stats;
}

} // namespace fractals::persist