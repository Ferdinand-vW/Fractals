#include <chrono>
#include <climits>
#include <ctime>
#include <numeric>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

#include <neither/either.hpp>

#include "fractals/app/Event.h"
#include "fractals/app/TorrentDisplayEntry.h"
#include "fractals/common/Tagged.h"
#include "fractals/persist/Event.h"
#include <fractals/torrent/MetaInfo.h>

namespace fractals::app
{

TorrentDisplayEntry::TorrentDisplayEntry(uint64_t entryId, const torrent::TorrentMeta &tm)
    : entryId(entryId), infoHash(tm.getInfoHash()), name(tm.getName()), size(tm.getSize())
{
}

TorrentDisplayEntry::TorrentDisplayEntry(uint64_t entryId, const persist::TorrentModel &tm)
    : entryId(entryId), infoHash(tm.infoHash), name(tm.name), size(tm.size)
{
}

void TorrentDisplayEntry::update(std::chrono::nanoseconds nanos, const persist::TorrentStats &stats)
{
    auto timeDiff = (nanos - prevTime).count();
    downloadSpeed = timeDiff > 0 ? (stats.downloaded - downloaded) * 1e9 / timeDiff : 0;
    uploadSpeed = timeDiff > 0 ? (stats.uploaded - uploaded) * 1e9 / timeDiff : 0;
    downloaded = stats.downloaded;
    uploaded = stats.uploaded;
    prevTime = nanos;
}

void TorrentDisplayEntry::update(const app::PeerStats& stats)
{
    connectedSeeders = stats.connectedPeersCount;
    totalSeeders = stats.knownPeerCount;
    connectedLeechers = 0;
    totalLeechers = 0;
}

uint64_t TorrentDisplayEntry::getId() const
{
    return entryId;
}

const common::InfoHash &TorrentDisplayEntry::getInfoHash() const
{
    return infoHash;
}

TorrentDisplayEntry::State TorrentDisplayEntry::getState() const
{
    return state;
}

void TorrentDisplayEntry::setRunning()
{
    state = State::Running;
}

void TorrentDisplayEntry::setStopped()
{
    state = State::Stopped;
}

void TorrentDisplayEntry::setCompleted()
{
    state = State::Completed;
}

bool TorrentDisplayEntry::isDownloadComplete() const
{
    return downloaded == size;
}

const std::string &TorrentDisplayEntry::getName() const
{
    return name;
}

uint64_t TorrentDisplayEntry::getSize() const
{
    return size;
}

uint64_t TorrentDisplayEntry::getDownloaded() const
{
    return downloaded;
}

uint64_t TorrentDisplayEntry::getUploaded() const
{
    return uploaded;
}

uint64_t TorrentDisplayEntry::getDownloadSpeed() const
{
    return downloadSpeed;
}

uint64_t TorrentDisplayEntry::getUploadSpeed() const
{
    return uploadSpeed;
}

uint64_t TorrentDisplayEntry::getTotalSeeders() const
{
    return totalSeeders;
}

uint64_t TorrentDisplayEntry::getConnectedSeeders() const
{
    return connectedSeeders;
}

uint64_t TorrentDisplayEntry::getTotalLeechers() const
{
    return totalLeechers;
}

uint64_t TorrentDisplayEntry::getConnectedLeechers() const
{
    return connectedLeechers;
}

uint64_t TorrentDisplayEntry::getEta() const
{
    auto total = getSize();
    auto downloaded = getDownloaded();

    if (downloadSpeed <= 0)
    {
        return LLONG_MAX;
    }

    return (total - downloaded) / downloadSpeed;
}

} // namespace fractals::app