#pragma once

#include "fractals/common/Tagged.h"
#include "fractals/network/http/Announce.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/network/http/Event.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerClient.h"
#include "fractals/sync/QueueCoordinator.h"

#include <chrono>
#include <ratio>
#include <unordered_map>

namespace fractals::network::http
{

class TrackerState
{
  public:
    TrackerState() = default;

    void reset();
    bool canAnnounce(std::chrono::nanoseconds now) const;

    void notifyOfRequest();
    void notifyOfRetry();
    void resetRetryCount();
    uint64_t getRetryCount() const;
    void setInterval(uint64_t minInterval, uint64_t interval);
    std::vector<PeerId> onAnnounce(const http::Announce& ann);

    void update(std::chrono::nanoseconds now);

  private:
    uint64_t minInterval{0};
    uint64_t interval{0};
    uint64_t requestNum{0};
    uint64_t retryCount{0};
    std::chrono::nanoseconds prevRequestTime{0};
    std::unordered_set<PeerId> peers;
};

template <typename TrackerClientT> class AnnounceServiceImpl
{
  public:
    AnnounceServiceImpl(sync::QueueCoordinator &coordinator,
                        AnnounceEventQueue::RightEndPoint queue, TrackerClientT &client);

    TrackerClientT &getClient();

    void run();
    void pollOnce();
    void disable();

  private:
    void process(const RequestAnnounce &tr);
    void process(const AddTrackers &at);
    void process(const DeleteTrackers &req);
    void process(const Pause &req);
    void process(const Shutdown&);

    void execute(const std::string& announce, const RequestAnnounce& req);

    std::optional<std::string> getNextAnnounce(const common::InfoHash &infoHash,
                                               std::chrono::nanoseconds now);
    std::optional<std::pair<std::string, RequestAnnounce>> getExecutableDelayedRequest();

    bool running{true};

    AnnounceEventQueue::RightEndPoint requestQueue;
    TrackerClientT &client;
    uint32_t outstandingRequests{0};
    sync::QueueCoordinator &coordinator;

    std::unordered_map<common::InfoHash, RequestAnnounce> delayedRequests;
    std::unordered_map<std::string, RequestAnnounce> executedRequests;
    std::unordered_map<std::string, TrackerState> trackerStates;
    std::unordered_set<common::InfoHash> subscribers;
    std::unordered_map<common::InfoHash, std::vector<std::string>> trackerSet;
};

using AnnounceService = AnnounceServiceImpl<TrackerClient>;

} // namespace fractals::network::http