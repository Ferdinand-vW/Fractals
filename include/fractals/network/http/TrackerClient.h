#pragma once

#include "fractals/common/CurlPoll.h"
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
        PollResult(const std::string &infoHash, const std::string error) : infoHash(infoHash), error(error){};
        PollResult(const std::string &infoHash, const TrackerResponse &resp) : infoHash(infoHash), response(resp){};

        std::string infoHash;
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
        std::unordered_map<uint64_t, std::string> nameMap;
};
} // namespace fractals::network::http