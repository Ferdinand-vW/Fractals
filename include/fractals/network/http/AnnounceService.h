#pragma once

#include "fractals/network/http/Announce.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerClient.h"
#include "fractals/network/http/RequestAnnounceQueue.h"

#include <unordered_map>

namespace fractals::network::http
{

template<typename TrackerClientT>
class AnnounceServiceImpl
{
  public:
    AnnounceServiceImpl(RequestAnnounceQueue::RightEndPoint queue, TrackerClientT &client);

    TrackerClientT& getClient();

    void pollForever();
    bool pollOnce();
    void disable();

    void subscribe(const std::string& infoHash);
    void unsubscribe(const std::string& infoHash);

    bool isSubscribed(const std::string& infoHash) const;

  private:
    bool running{true};

    RequestAnnounceQueue::RightEndPoint requestQueue;
    TrackerClientT& client;

    std::unordered_set<std::string> subscribers;
};

using AnnounceService = AnnounceServiceImpl<TrackerClient>;

} // namespace fractals::network::http