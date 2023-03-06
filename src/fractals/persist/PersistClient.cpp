#include <mutex>
#include <optional>

#include "fractals/common/utils.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/Models.h"
#include "fractals/persist/PersistClient.h"

namespace fractals::persist
{

using namespace sqlite_orm;

void PersistClient::openConnection(const std::string &connString)
{
    if (!dbConn || !dbConn->is_opened())
    {
        // connect to user given db and create tables if not already present
        auto db = makeDatabase(connString);
        // update shared pointer to point to new database connection
        dbConn = std::make_shared<Schema>(db);
    }

    dbConn->sync_schema();
}

void PersistClient::addTorrent(const AddTorrent &at)
{
    const auto torrId = dbConn->insert(TorrentModel{-1, at.name, at.torrentFilePath, at.writePath});
    torrentIds.emplace(at.infoHash, torrId);
}

std::optional<TorrentModel> PersistClient::loadTorrent(const std::string &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        auto tms = dbConn->get_all<TorrentModel>(where(c(&TorrentModel::id) == torrId->second));

        if (tms.begin() != tms.end())
        {
            return tms.front();
        }
    }

    return std::nullopt;
}

std::vector<TorrentModel> PersistClient::loadTorrents()
{
    return dbConn->get_all<TorrentModel>();
}

void PersistClient::deleteTorrent(const std::string &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->remove<TorrentModel>(torrId->second);
        torrentIds.erase(infoHash);
    }
}

void PersistClient::addPiece(const AddPiece &pm)
{
    auto torrId = torrentIds.find(pm.infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->insert(PieceModel{-1, torrId->second, pm.piece});
    }
}

std::vector<PieceModel> PersistClient::loadPieces(const std::string &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        return dbConn->get_all<PieceModel>(where(c(&PieceModel::torrent_id) == torrId->second));
    }

    return {};
}

void PersistClient::deletePieces(const std::string &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->remove_all<PieceModel>(where(c(&PieceModel::torrent_id) == torrId->second));
    }
}

void PersistClient::addAnnounce(const AddAnnounce &ann)
{
    auto torrId = torrentIds.find(ann.infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->insert(AnnounceModel{-1, torrId->second, ann.peerIp, ann.peerPort, ann.announceTime, ann.interval, ann.minInterval, false});
    }
}

void PersistClient::deleteAnnounce(const std::string &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        dbConn->remove_all<AnnounceModel>(where(c(&AnnounceModel::torrent_id) == torrId->second));
    }
}

std::vector<AnnounceModel> PersistClient::loadAnnounces(const std::string &infoHash)
{
    auto torrId = torrentIds.find(infoHash);
    if (torrId != torrentIds.end())
    {
        return dbConn->get_all<AnnounceModel>(where(c(&AnnounceModel::torrent_id) == torrId->second));
    }

    return {};
}

} // namespace fractals::persist