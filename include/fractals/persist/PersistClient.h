#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#include "fractals/persist/Event.h"
#include "fractals/persist/Models.h"

namespace fractals::persist
{
/**
Simple type that represents the database schema
*/
class PersistClient
{
  public:
    PersistClient() = default;
    void openConnection(const std::string& connString);

    void addTorrent(const AddTorrent &t);
    std::optional<TorrentModel> loadTorrent(const std::string &infoHash);
    std::vector<TorrentModel> loadTorrents();
    void deleteTorrent(const std::string &infoHash);

    void addPiece(const AddPiece &pm);
    std::vector<PieceModel> loadPieces(const std::string &infoHash);
    void deletePieces(const std::string &infoHash);

    void addAnnounce(const AddAnnounce &ann);
    void deleteAnnounce(const std::string &infoHash);
    std::vector<AnnounceModel> loadAnnounces(const std::string &infoHash);

  private:
    std::unordered_map<std::string, int> torrentIds;
    std::shared_ptr<Schema> dbConn;
};

} // namespace fractals::persist