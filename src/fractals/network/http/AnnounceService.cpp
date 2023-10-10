#include "fractals/network/http/AnnounceService.h"
#include <chrono>
#include <fractals/network/http/AnnounceService.ipp>

namespace fractals::network::http
{
void TrackerState::reset()
{
    peers = {};
    prevRequestTime = std::chrono::nanoseconds{0};
    retryCount = 0;
}

bool TrackerState::canAnnounce(std::chrono::nanoseconds now) const
{
    return now.count() > prevRequestTime.count() + minInterval;
}

void TrackerState::setInterval(uint64_t minInterval, uint64_t interval)
{
    this->minInterval = minInterval;
    this->interval = interval;
}

void TrackerState::notifyOfRequest()
{
    ++requestNum;
}

void TrackerState::notifyOfRetry()
{
    ++retryCount;
}

void TrackerState::resetRetryCount()
{
    retryCount = 0;
}

uint64_t TrackerState::getRetryCount() const
{
    return retryCount;
}

void TrackerState::update(std::chrono::nanoseconds now)
{
    this->prevRequestTime = now;
}

std::vector<PeerId> TrackerState::onAnnounce(const http::Announce& ann)
{
    const auto currSize = peers.size();

    std::vector<PeerId> newPeers;
    for (const auto &p : ann.peers)
    {
        if (!peers.contains(p))
        {
            peers.emplace(p);
            newPeers.emplace_back(p);
        }
    }

    return newPeers;
}

template class AnnounceServiceImpl<TrackerClient>;
} // namespace fractals::network::http