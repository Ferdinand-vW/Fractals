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

            neither::Either<std::string, TrackerResponse> query(const TrackerRequest &tr, std::chrono::milliseconds recvTimeout);

            friend std::ostream & operator<<(std::ostream& out, const TrackerClient & s);

        private:
            std::string mUrl;

    };
}