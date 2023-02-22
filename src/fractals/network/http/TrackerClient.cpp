#include "fractals/network/http/TrackerClient.h"
#include "fractals/network/http/Request.h"

#include <curl/curl.h>
#include <sstream>

namespace fractals::network::http
{
    void TrackerClient::query(const TrackerRequest &tr, std::chrono::milliseconds recvTimeout)
    {
        auto reqId = poller.add(tr.toHttpGetUrl(), recvTimeout);
        nameMap.emplace(reqId, tr.url_info_hash);
    }

    TrackerClient::PollResult TrackerClient::poll()
    {
        common::CurlResponse resp = poller.poll();
        const auto reqId = resp.getRequestId();
        if(resp && nameMap.contains(reqId))
        {
            std::stringstream ss(std::string(resp.getData().begin(), resp.getData().end()));

            auto bencodeRes = bencode::decode<bencode::bdict>(ss);

            if (!bencodeRes.has_value()) 
            { 
                return TrackerClient::PollResult(nameMap[reqId], "Received invalid bencoded tracker reponse");
            }

            auto bencoded = bencodeRes.value();

            const auto decodedResponse = TrackerResponse::decode(bencoded);

            if (decodedResponse.index() == 0)
            {
                return TrackerClient::PollResult(nameMap[reqId],std::get<TrackerResponse>(decodedResponse));
            }
        }

        return PollResult("", resp.getError());
    }
}