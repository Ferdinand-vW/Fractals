#pragma once

#include "fractals/network/http/Announce.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/Tracker.h"
#include "fractals/network/http/TrackerRequestQueue.h"

#include <unordered_map>

namespace fractals::network::http
{

    class TrackerService
    {
        public:
            TrackerService();

            TrackerRequestQueue& getRequestQueue();

            void stop();
        private:
            void run();

            bool running{true};

            TrackerRequestQueue requestQueue;
            TrackerClient client;

            std::unordered_map<std::string, std::function<void(const Announce&)>> subscribers;
    };

}