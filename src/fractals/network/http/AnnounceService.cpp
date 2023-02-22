#include "fractals/network/http/AnnounceService.h"
#include "fractals/common/CurlPoll.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerRequestQueue.h"
#include <chrono>
#include <ctime>
#include <thread>

namespace fractals::network::http
{
    AnnounceService::AnnounceService() : client({})
    {
        run();
    }

    TrackerRequestQueue& AnnounceService::getRequestQueue()
    {
        return requestQueue;
    }

    void AnnounceService::stop()
    {
        running = false;
    }

    void AnnounceService::subscribe(const std::string& infoHash, std::function<void(const Announce&)> callback)
    {
        subscribers.emplace(infoHash, callback);
    }

    void AnnounceService::unsubscribe(const std::string& infoHash)
    {
        subscribers.erase(infoHash);
    }

    void AnnounceService::run()
    {
        running = true;

        while(running)
        {
            if (!requestQueue.isEmpty())
            {
                const auto tm = requestQueue.pop();
                TrackerRequest req(tm.getMetaInfo());

                client.query(req, std::chrono::milliseconds(5000));
            }

            TrackerClient::PollResult resp = client.poll();

            if (resp)
            {
                auto it = subscribers.find(resp.infoHash);

                if (it != subscribers.end())
                {
                    const auto now = time(nullptr);
                    it->second(resp.response->toAnnounce(now));
                }
                else
                {
                    // TODO: Iterate over subscribers to see if any should be timed out
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

}