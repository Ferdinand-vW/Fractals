#pragma once

#include "fractals/network/http/Request.h"
#include <chrono>
#include <ratio>

namespace fractals::network::http
{
    class TrackerClient
    {
        public:
            TrackerResult query(const TrackerRequest& req, std::chrono::milliseconds recvTimeout);
    };
}