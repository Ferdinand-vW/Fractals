#include "AnnounceService.h"

#include "fractals/AppId.h"
#include "fractals/common/CurlPoll.h"
#include "fractals/common/Tagged.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/network/http/AnnounceService.h"
#include "fractals/network/http/Event.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerClient.h"
#include "fractals/sync/QueueCoordinator.h"
#include <chrono>
#include <ctime>
#include <thread>

namespace fractals::network::http
{
template <typename TrackerClientT>
AnnounceServiceImpl<TrackerClientT>::AnnounceServiceImpl(sync::QueueCoordinator &coordinator,
                                                         AnnounceEventQueue::RightEndPoint queue,
                                                         TrackerClientT &client)
    : coordinator(coordinator), requestQueue(queue), client(client)
{
    coordinator.addAsPublisherForAnnounceService<AnnounceEventQueue>(queue);
}

template <typename TrackerClientT> TrackerClientT &AnnounceServiceImpl<TrackerClientT>::getClient()
{
    return client;
}

template <typename TrackerClientT> void AnnounceServiceImpl<TrackerClientT>::disable()
{
    running = false;
}

template <typename TrackerClientT> void AnnounceServiceImpl<TrackerClientT>::run()
{
    running = true;
    spdlog::info("AS::run. Starting announce service");
    while (running)
    {
        if (!executedRequests.empty() || !delayedRequests.empty())
        {
            coordinator.waitOnAnnounceServiceUpdate(std::chrono::milliseconds(500));
        }
        else
        {
            coordinator.waitOnAnnounceServiceUpdate();
        }

        pollOnce();
    }

    spdlog::info("AS::run. Shutdown");
}

template <typename TrackerClientT> void AnnounceServiceImpl<TrackerClientT>::pollOnce()
{
    if (requestQueue.canPop())
    {
        std::visit(common::overloaded{[this](const auto &req)
                                      {
                                          process(req);
                                      }},
                   requestQueue.pop());
    }

    while (const auto req = getExecutableDelayedRequest())
    {
        delayedRequests.erase(req->second.infoHash);
        execute(req->first, req->second);
    }

    TrackerClient::PollResult resp = client.poll();

    if (resp.announce.empty())
    {
        return;
    }

    spdlog::info("AnnService::poll resp.announce={}", resp.announce);

    const std::string &announce = resp.announce;
    const auto &infoHash = resp.infoHash;

    assert(trackerStates.contains(announce));
    TrackerState &trackerState = trackerStates[announce];

    if (resp && subscribers.contains(infoHash))
    {
        const auto req = executedRequests.find(resp.announce)->second;
        executedRequests.erase(announce);

        const time_t now = time(nullptr);
        trackerState.setInterval(resp.response->minInterval, resp.response->interval);
        trackerState.update(std::chrono::system_clock::from_time_t(now).time_since_epoch());

        auto announceResponse = resp.response->toAnnounce(infoHash, now);
        const auto newPeers = trackerState.onAnnounce(announceResponse);

        if (!newPeers.empty())
        {
            announceResponse.peers = newPeers;
            requestQueue.push(std::move(announceResponse));
            subscribers.erase(infoHash);
        }
        else
        {
            delayedRequests.emplace(resp.infoHash, req);
            trackerState.notifyOfRetry();
        }
    }
    else if (!resp.error.empty())
    {
        spdlog::info("AS::pollOnce. Error response {} InfoHash={}", resp.error, resp.infoHash);

        const auto req = executedRequests.find(resp.announce)->second;
        assert(req.infoHash == resp.infoHash);

        executedRequests.erase(resp.announce);
        delayedRequests.emplace(resp.infoHash, req);

        trackerState.notifyOfRetry();
    }
    else
    {
        executedRequests.erase(resp.announce);
    }
}

template <typename TrackerClientT>
void AnnounceServiceImpl<TrackerClientT>::process(const RequestAnnounce &req)
{
    spdlog::info("AS::process(RequestAnnounce) Beg. infoHash={}", req.infoHash);

    if (subscribers.contains(req.infoHash))
    {
        spdlog::warn("AS::process(RequestAnnounce). InfoHash already subscribed");
        return;
    }

    subscribers.emplace(req.infoHash);

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto announce = getNextAnnounce(req.infoHash, now);
    if (announce)
    {
        spdlog::warn("AS::process(RequestAnnounce). Execute");
        execute(announce.value(), req);
    }
    else
    {
        spdlog::info("AS::process(RequestAnnounce). Delay announce");
        delayedRequests.emplace(req.infoHash, req);
    }
}

template <typename TrackerClientT>
void AnnounceServiceImpl<TrackerClientT>::process(const AddTrackers &req)
{
    spdlog::info("AS::process(AddTrackers) InfoHash={}", req.infoHash);
    if (req.trackers.empty())
    {
        spdlog::error("AS::process(AddTrackers). Empty tracker list");
        return;
    }

    for (const auto &ann : req.trackers)
    {
        if (!ann.url.starts_with("udp"))
        {
            trackerSet[req.infoHash].emplace_back(ann.url);
            trackerStates.emplace(ann.url, TrackerState{});
        }
    }
}

template <typename TrackerClientT>
void AnnounceServiceImpl<TrackerClientT>::process(const DeleteTrackers &req)
{
    spdlog::info("AS::process(DeleteTrackers) InfoHash={}", req.infoHash);
    trackerSet.erase(req.infoHash);
    delayedRequests.erase(req.infoHash);
    subscribers.erase(req.infoHash);
}

template <typename TrackerClientT>
void AnnounceServiceImpl<TrackerClientT>::process(const Pause &req)
{
    spdlog::info("AS::process(Pause) InfoHash={}", req.infoHash);
    for (auto& [_, ts] : trackerStates)
    {
        ts.reset();
    }
    delayedRequests.erase(req.infoHash);
    subscribers.erase(req.infoHash);
}

template <typename TrackerClientT>
void AnnounceServiceImpl<TrackerClientT>::process(const Shutdown &)
{
    running = false;
}

template <typename TrackerClientT>
void AnnounceServiceImpl<TrackerClientT>::execute(const std::string &announce,
                                                  const RequestAnnounce &req)
{
    client.query(TrackerRequest(announce, req.torrent, Fractals::APPID), std::chrono::milliseconds(5000));

    executedRequests.emplace(announce, req);
    trackerStates[announce].notifyOfRequest();
    spdlog::info("AS::execute. Perform query announce={} hash={}.", announce, req.infoHash);
}

template <typename TrackerClientT>
std::optional<std::string>
AnnounceServiceImpl<TrackerClientT>::getNextAnnounce(const common::InfoHash &infoHash,
                                                     std::chrono::nanoseconds now)
{
    if (!trackerSet.contains(infoHash))
    {
        spdlog::warn("AS::getNextAnnounce. No known trackers for infoHash={}", infoHash);
        return std::nullopt;
    }

    std::optional<std::string> result = std::nullopt;
    assert(trackerSet.contains(infoHash));
    auto &trackers = trackerSet[infoHash];

    uint32_t trackerIdx{0};
    while (trackerIdx < trackers.size() && !result.has_value())
    {

        const auto announce = trackers.front();

        assert(trackerStates.contains(announce));
        auto &tracker = trackerStates[announce];

        if (tracker.canAnnounce(now) && tracker.getRetryCount() <= 2)
        {
            result = announce;
        }
        else
        {
            trackers.erase(trackers.begin());
            trackers.push_back(announce);
        }

        ++trackerIdx;
    }

    return result;
}

template <typename TrackerClientT>
std::optional<std::pair<std::string, RequestAnnounce>>
AnnounceServiceImpl<TrackerClientT>::getExecutableDelayedRequest()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    for (const auto &[infoHash, req] : delayedRequests)
    {
        // assert(infoHash == req.infoHash);
        const auto ann = getNextAnnounce(infoHash, now);
        if (ann)
        {
            return std::make_pair(ann.value(), req);
        }
    }

    return std::nullopt;
}

} // namespace fractals::network::http