#pragma once

#include "fractals/common/CurlPoll.h"
#include "fractals/common/Tagged.h"
#include "fractals/network/http/Request.h"
#include <chrono>
#include <ratio>

namespace fractals::network::http
{
class TrackerClient
{
  public:
    struct PollResult
    {
        PollResult() = default;
        PollResult(const common::InfoHash &infoHash, const std::string &announce,
                   const std::string error)
            : infoHash(infoHash), announce(announce), error(error){};
        PollResult(const common::InfoHash &infoHash, const std::string &announce,
                   const TrackerResponse &resp)
            : infoHash(infoHash), announce(announce), response(resp){};

        common::InfoHash infoHash{};
        std::string announce{""};
        std::string error{""};
        std::optional<TrackerResponse> response;

        operator bool() const
        {
            return response.has_value();
        }
    };

    void query(const TrackerRequest &req, std::chrono::milliseconds recvTimeout);

    PollResult poll();

  private:
    common::CurlPoll poller;
    std::unordered_map<uint64_t, std::pair<common::InfoHash, std::string>> nameMap;
};
} // namespace fractals::network::http