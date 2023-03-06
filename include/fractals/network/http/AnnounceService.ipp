#include "AnnounceService.h"

#include "fractals/common/CurlPoll.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerClient.h"
#include <chrono>
#include <ctime>
#include <thread>

namespace fractals::network::http
{
template <typename TrackerClientT>
AnnounceServiceImpl<TrackerClientT>::AnnounceServiceImpl(AnnounceEventQueue::RightEndPoint queue,
                                                         TrackerClientT &client)
    : requestQueue(queue), client(client)
{
}

template <typename TrackerClientT> TrackerClientT &AnnounceServiceImpl<TrackerClientT>::getClient()
{
    return client;
}

template <typename TrackerClientT> void AnnounceServiceImpl<TrackerClientT>::disable()
{
    running = false;
}

template <typename TrackerClientT> void AnnounceServiceImpl<TrackerClientT>::subscribe(const std::string &infoHash)
{
    subscribers.emplace(infoHash);
}

template <typename TrackerClientT> void AnnounceServiceImpl<TrackerClientT>::unsubscribe(const std::string &infoHash)
{
    subscribers.erase(infoHash);
}

template <typename TrackerClientT>
bool AnnounceServiceImpl<TrackerClientT>::isSubscribed(const std::string &infoHash) const
{
    return subscribers.contains(infoHash);
}

template <typename TrackerClientT> void AnnounceServiceImpl<TrackerClientT>::pollForever()
{
    running = true;
    while (running)
    {
        if (!pollOnce())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

template <typename TrackerClientT> bool AnnounceServiceImpl<TrackerClientT>::pollOnce()
{
    if (requestQueue.canPop())
    {
        const auto req = requestQueue.pop();

        client.query(req, std::chrono::milliseconds(5000));
    }

    TrackerClient::PollResult resp = client.poll();

    if (resp)
    {
        auto it = subscribers.find(resp.infoHash);

        if (it != subscribers.end())
        {
            const auto now = time(nullptr);
            requestQueue.push(std::move(resp.response->toAnnounce(now)));
        }
        else
        {
            // TODO: Iterate over subscribers to see if any should be timed out
        }

        return true;
    }

    return false;
}

} // namespace fractals::network::http