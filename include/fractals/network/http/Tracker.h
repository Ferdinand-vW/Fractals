#pragma once

#include <chrono>
#include <string>
#include <sys/types.h>
#include <iosfwd>
#include <vector>
#include <optional>

#include <neither/either.hpp>
#include <neither/maybe.hpp>
#include <neither/traits.hpp>

#include "fractals/network/http/Peer.h"
#include "fractals/network/http/Request.h"

namespace fractals::torrent { class MetaInfo; }

namespace fractals::network::http {

    struct Announce;


    struct TrackerClient {
        public:
            TrackerClient(std::string &&url);
            TrackerClient(const std::string &url);
            std::string getUrl() const;

            TrackerRequest query(const TrackerRequest &tr, std::chrono::milliseconds recvTimeout);

            friend std::ostream & operator<<(std::ostream& out, const TrackerClient & s);

        private:
            std::string mUrl;

    };

    /**
    How to perform an announce:
    1) The MetaInfo file is to be converted to a TrackerRequest using @makeTrackerRequest.
    2) Send the tracker request to the tracker and receive the tracker response using @sendTrackerRequest.
    3) Convert to tracker response to an announce object using @toAnnounce
    */
    TrackerRequest makeTrackerRequest(const torrent::MetaInfo & mi);

}