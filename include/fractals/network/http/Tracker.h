#pragma once

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

    struct ITrackerClient
    {
        virtual neither::Either<std::string, TrackerResponse> sendRequest(const TrackerRequest &tr) = 0;
    };

    struct TrackerClient :  ITrackerClient {
        public:
            TrackerClient(std::string &&url);
            TrackerClient(const std::string &url);
            std::string getUrl() const;

            neither::Either<std::string, TrackerResponse> sendRequest(const TrackerRequest &tr) override;

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