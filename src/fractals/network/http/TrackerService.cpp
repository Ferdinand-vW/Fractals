#include "fractals/network/http/TrackerService.h"
#include "fractals/network/http/TrackerRequestQueue.h"
#include <chrono>
#include <ctime>
#include <thread>

namespace fractals::network::http
{
    TrackerService::TrackerService() : client({})
    {
        run();
    }

    TrackerRequestQueue& TrackerService::getRequestQueue()
    {
        return requestQueue;
    }

    void TrackerService::stop()
    {
        running = false;
    }

    void TrackerService::run()
    {
        running = true;

        while(running)
        {
            if (!requestQueue.isEmpty())
            {
                const auto tm = requestQueue.pop();

                TrackerRequest req(tm.getMetaInfo());

                const auto resp = client.query(req, std::chrono::milliseconds(5000));

                if (resp)
                {
                    auto it = subscribers.find(tm.getName());

                    if (it != subscribers.end())
                    {
                        const auto now = time(nullptr);
                        it->second(resp.response->toAnnounce(now));
                    }
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

}