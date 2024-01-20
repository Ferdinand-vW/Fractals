#pragma once

#include <fractals/app/Event.h>
#include <fractals/common/Tagged.h>
#include <fractals/persist/Event.h>
#include <fractals/persist/Models.h>
#include <fractals/torrent/TorrentMeta.h>
#include <chrono>
#include <memory>
#include <optional>
#include <ratio>
#include <string>
#include <sys/types.h>

namespace fractals::app
{

/**
View of active BitTorrent connection.
Responsible for calculating various statistics (e.g. download speed)
*/
class TorrentDisplayEntry
{
  public:
    enum State { Running, Completed, Stopped };

    TorrentDisplayEntry() = default;
    TorrentDisplayEntry(uint64_t entryId, const torrent::TorrentMeta&);
    TorrentDisplayEntry(uint64_t entryId, const persist::TorrentModel&);

    void update(std::chrono::nanoseconds nanos, const persist::TorrentStats& stats);
    void update(const app::PeerStats& stats);

    uint64_t getId() const;
    const common::InfoHash& getInfoHash() const;
    State getState() const;
    void setRunning();
    void setStopped();
    void setCompleted();
    bool isDownloadComplete() const;
    const std::string& getName() const;
    uint64_t getSize() const;
    uint64_t getDownloaded() const;
    uint64_t getUploaded() const;
    // speeds are reported in number of bytes per second
    uint64_t getDownloadSpeed() const;
    uint64_t getUploadSpeed() const;
    uint64_t getTotalSeeders() const;
    uint64_t getConnectedSeeders() const;
    uint64_t getTotalLeechers() const;
    uint64_t getConnectedLeechers() const;
    uint64_t getEta() const;

  private:
    uint64_t entryId;
    common::InfoHash infoHash;
    State state{Stopped};
    std::string name{""};
    uint64_t size{0};
    uint64_t downloaded{0};
    uint64_t uploaded{0};
    uint64_t totalSeeders{0};
    uint64_t connectedSeeders{0};
    uint64_t totalLeechers{0};
    uint64_t connectedLeechers{0};

    std::chrono::nanoseconds prevTime{0};
    uint64_t downloadSpeed{0};
    uint64_t uploadSpeed{0};
};

} // namespace fractals::app