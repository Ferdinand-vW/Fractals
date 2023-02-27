#pragma once

#include "fractals/network/http/Announce.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerClient.h"
#include "fractals/network/http/TrackerRequestQueue.h"

#include <unordered_map>

namespace fractals::network::http
{

template<typename TrackerClientT>
class AnnounceServiceImpl
{
  public:
    AnnounceServiceImpl(TrackerRequestQueue& queue, TrackerClientT &client);

    TrackerRequestQueue &getRequestQueue();
    TrackerClientT& getClient();

    void pollForever();
    bool pollOnce();
    void disable();

    void subscribe(const std::string& infoHash, std::function<void(const Announce&)> callback);
    void unsubscribe(const std::string& infoHash);

    bool isSubscribed(const std::string& infoHash) const;

  private:
    bool running{true};

    TrackerRequestQueue& requestQueue;
    TrackerClientT& client;

    std::unordered_map<std::string, std::function<void(const Announce &)>> subscribers;
};

using AnnounceService = AnnounceServiceImpl<TrackerClient>;

} // namespace fractals::network::http