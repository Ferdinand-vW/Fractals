#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#include <fractals/common/Tagged.h>
#include <fractals/persist/Event.h>
#include <fractals/persist/Models.h>

namespace fractals::persist
{
/**
Simple type that represents the database schema
*/
class PersistClient
{
  public:
    PersistClient() = default;
    void openConnection(const std::string &connString);

    std::variant<AddedTorrent, TorrentExists> addTorrent(const AddTorrent &t);
    std::optional<TorrentModel> loadTorrent(const common::InfoHash &infoHash);
    std::vector<std::pair<TorrentModel, std::vector<FileModel>>> loadTorrents();
    void deleteTorrent(const common::InfoHash &infoHash);

    void addTrackers(const AddTrackers &req);
    std::vector<TrackerModel> loadTrackers(const common::InfoHash &ih);

    void addPiece(const PieceComplete &pm);
    std::vector<PieceModel> loadPieces(const common::InfoHash &infoHash);
    void deletePieces(const common::InfoHash &infoHash);

    void addAnnounce(const AddAnnounce &ann);
    void deleteAnnounce(const common::InfoHash &infoHash);
    std::vector<AnnounceModel> loadAnnounces(const common::InfoHash &infoHash);

    TorrentStats loadTorrentStats(const common::InfoHash &infoHash);

  private:
    std::unordered_map<common::InfoHash, int> torrentIds;
    std::unordered_map<common::InfoHash, TorrentStats> inMemoryStats;
    std::unique_ptr<Schema> dbConn;
};

} // namespace fractals::persist