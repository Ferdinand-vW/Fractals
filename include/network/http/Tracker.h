#pragma once

#include "Peer.h"

#include <neither/neither.hpp>
#include <bencode/bencode.h>

#include <string>

class MetaInfo;
struct Announce;

using namespace neither;

enum class Event { Started, Stopped, Completed };

struct TrackerRequest {
    public:
        std::string announce;
        std::vector<char> info_hash;
        // urlencoded 20-byte hash of info key
        std::string url_info_hash;
        std::vector<char> peer_id;
        // urlencoded 20-byte string used as unique id for client
        std::string url_peer_id;
        // port number that client is listening on
        int port;
        // total amount uploaded in base 10 ASCII
        int uploaded;
        // total amount downloaded in base 10 ASCII
        int downloaded;
        // number of bytes this client still has to download in base 10 ASCII
        int left;
        // client accepts a compact response
        int compact;
        // Started on first request, Stopped if client is shutting down, Completed when downloaded completes
        // Maybe<Event> event;
        // Maybe<std::string> ip;
        // Maybe<std::int16_t> numwant;
        //key
        //trackerid
};

struct TrackerResponse {
    public:
        neither::Maybe<std::string> warning_message;
        int interval;
        neither::Maybe<int> min_interval;
        neither::Maybe<std::string> tracker_id;
        int complete;
        int incomplete;
        std::vector<Peer> peers;

        friend std::ostream & operator<<(std::ostream& out, const TrackerResponse & s);
};

TrackerRequest makeTrackerRequest(const MetaInfo & mi);

Announce toAnnounce(time_t now,const TrackerResponse &tr);
neither::Either<string,TrackerResponse> sendTrackerRequest(const TrackerRequest &tr);