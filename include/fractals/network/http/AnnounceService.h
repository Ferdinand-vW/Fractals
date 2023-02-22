#pragma once

#include "fractals/network/http/Announce.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerClient.h"
#include "fractals/network/http/TrackerRequestQueue.h"

#include <unordered_map>

namespace fractals::network::http
{

class AnnounceService
{
  public:
    AnnounceService();

    TrackerRequestQueue &getRequestQueue();

    void stop();

    void subscribe(const std::string& infoHash, std::function<void(const Announce&)> callback);
    void unsubscribe(const std::string& infoHash);

  private:
    void run();

    bool running{true};

    TrackerRequestQueue requestQueue;
    TrackerClient client;

    std::unordered_map<std::string, std::function<void(const Announce &)>> subscribers;
};

} // namespace fractals::network::http