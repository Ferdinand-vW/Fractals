#include "fractals/network/http/TrackerClient.h"
#include "fractals/network/http/Request.h"

#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <variant>

namespace fractals::network::http
{
void TrackerClient::query(const TrackerRequest &tr, std::chrono::milliseconds recvTimeout)
{
    auto reqId = poller.add(tr.toHttpGetUrl(), recvTimeout);
    nameMap.emplace(reqId, std::make_pair(tr.info_hash, tr.announce));
}

TrackerClient::PollResult TrackerClient::poll()
{
    common::CurlResponse resp = poller.poll();
    const auto reqId = resp.getRequestId();

    if (reqId == 0)
    {
        spdlog::debug("TrackerClient::poll. No result available yet");
        return PollResult({}, "", "");
    }
    if (!nameMap.contains(reqId))
    {
        spdlog::error("TrackerClient::poll. Unknown requestId={}", reqId);
        return PollResult({}, "", "");
    }

    const auto infoHash = nameMap[reqId].first;
    const auto announce = nameMap[reqId].second;

    if (resp)
    {
        std::stringstream ss(std::string(resp.getData().begin(), resp.getData().end()));

        auto bencodeRes = bencode::decode<bencode::bdict>(ss);
        if (!bencodeRes.has_value())
        {
            return PollResult(infoHash, announce,
                              "Received invalid bencoded tracker reponse error=" +
                                  bencodeRes.error().message());
        }

        auto bencoded = bencodeRes.value();
        const auto decodedResponse = TrackerResponse::decode(bencoded);

        if (std::holds_alternative<TrackerResponse>(decodedResponse))
        {
            return PollResult(infoHash, announce, std::get<TrackerResponse>(decodedResponse));
        }
        else
        {
            return PollResult(infoHash, announce, std::get<std::string>(decodedResponse));
        }
    }
    // no response yet
    return PollResult(infoHash, announce, resp.getError());
}
} // namespace fractals::network::http