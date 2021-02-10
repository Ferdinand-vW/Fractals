#pragma once

#include <string>
#include <neither/neither.hpp>
#include <bencode/bencode.h>

#include "torrent/MetaInfo.h"
#include "torrent/BencodeConvert.h"

using namespace neither;

enum class Event { Started, Stopped, Completed };

struct TrackerRequest {
    public:
        // urlencoded 20-byte hash of info key
        std::string info_hash;
        // urlencoded 20-byte string used as unique id for client
        std::string peer_id;
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

        static TrackerRequest make_request(MetaInfo mi);
};
