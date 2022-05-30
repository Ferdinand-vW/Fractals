#pragma once

#include <string>
#include <sys/types.h>
#include <iosfwd>
#include <vector>

#include <neither/either.hpp>
#include <neither/maybe.hpp>
#include <neither/traits.hpp>

#include "fractals/network/http/Peer.h"

namespace fractals::torrent { class MetaInfo; }

namespace fractals::network::http {

    struct Announce;

    struct Tracker {
        public:
            Tracker(std::string &&url);
            Tracker(const std::string &url);
            std::string getUrl() const;

            friend std::ostream & operator<<(std::ostream& out, const Tracker & s);

        private:
            std::string mUrl;
    };

    /**
    Request to be sent to the tracker
    */
    struct TrackerRequest {
        public:
            std::string announce;

            /**
            20-byte SHA1 hash of info value in MetaInfo file
            */
            std::vector<char> info_hash; 

            /**
            URL encoding of @info_hash
            */
            std::string url_info_hash;
            
            /**
            20-byte string used as unique id for client
            */
            std::vector<char> peer_id;

            /**
            URL encoding of 20-byte string used as unique id for client
            */
            std::string url_peer_id;

            /** 
            port number that client is listening on
            */
            int port;

            /** 
            total amount uploaded in base 10 ASCII
            */
            int uploaded;
            
            /** 
            total amount downloaded in base 10 ASCII
            */
            int downloaded;
            
            /** 
            number of bytes this client still has to download in base 10 ASCII
            */
            int left;

            
            /** 
            client accepts a compact response
            */
            int compact;

            // optional keys that we don't currently use
            //
            // Started on first request, Stopped if client is shutting down, Completed when downloaded completes
            // Maybe<Event> event;
            // Maybe<std::string> ip;
            // Maybe<std::int16_t> numwant;
            //
            //
    };

    /**
    Expected response from tracker
    */
    struct TrackerResponse {
        public:
            /**
            Populated if tracker returns error
            */
            neither::Maybe<std::string> warning_message;

            /**
            Expected time interval between each tracker request 
            */
            int interval;

            /**
            Minimum time interval allowed between each tracker request
            */
            neither::Maybe<int> min_interval;

            neither::Maybe<std::string> tracker_id;
            
            /**
            Number of seeders
            */
            int complete;

            /**
            Number of leechers
            */
            int incomplete;

            /**
            Set of peers that we can try connecting to
            */
            std::vector<Peer> peers;

            friend std::ostream & operator<<(std::ostream& out, const TrackerResponse & s);
    };

    /**
    How to perform an announce:
    1) The MetaInfo file is to be converted to a TrackerRequest using @makeTrackerRequest.
    2) Send the tracker request to the tracker and receive the tracker response using @sendTrackerRequest.
    3) Convert to tracker response to an announce object using @toAnnounce
    */
    TrackerRequest makeTrackerRequest(const torrent::MetaInfo & mi);
    neither::Either<std::string,TrackerResponse> sendTrackerRequest(const TrackerRequest &tr);
    Announce toAnnounce(time_t now,const TrackerResponse &tr);

}