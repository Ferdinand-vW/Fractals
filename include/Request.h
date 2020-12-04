#ifndef REQUEST_H

#include <string>
#include <optional>

enum class Event { Started, Stopped, Completed };

struct TrackerRequest {
    // urlencoded 20-byte hash of info key
    std::string info_hash;
    // urlencoded 20-byte string used as unique id for client
    std::string peer_id;
    // port number that client is listening on
    std::int16_t port;
    // total amount uploaded in base 10 ASCII
    std::int32_t uploaded;
    // total amount downloaded in base 10 ASCII
    std::int32_t downloaded;
    // number of bytes this client still has to download in base 10 ASCII
    std::int32_t left;
    // client accepts a compact response
    std::optional<bool> compact;
    // Started on first request, Stopped if client is shutting down, Completed when downloaded completes
    std::optional<Event> event;
    std::optional<std::string> ip;
    std::optional<std::int16_t> numwant;
    //key
    //trackerid
};

#endif
