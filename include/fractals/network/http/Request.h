#pragma once

#include "Announce.h"
#include "Peer.h"
#include "fractals/common/Tagged.h"
#include "fractals/persist/Models.h"
#include "fractals/torrent/MetaInfo.h"

#include <bencode/bencode.h>
#include <neither.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace fractals::network::http
{
/**
    Request to be sent to the tracker
    */
struct TrackerRequest
{
  public:
    TrackerRequest() = default;
    TrackerRequest(const std::string &announce, const torrent::MetaInfo &metaInfo, const common::AppId& appId);
    TrackerRequest(const std::string &announce, const persist::TorrentModel &model, const common::AppId& appId);
    TrackerRequest(const std::string &announce,
                   const common::InfoHash &infoHash, const std::string urlInfoHash,
                   const common::AppId& appId, const std::string urlPeerId, int port,
                   int uploaded, int downloaded, int left, int compact);

    std::string announce;

    /**
    20-byte SHA1 hash of info value in MetaInfo file
    */
    common::InfoHash info_hash;

    /**
    URL encoding of @info_hash
    */
    std::string url_info_hash;

    /**
    20-byte string used as unique id for client
    */
    common::AppId appId;

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
    // Started on first request, Stopped if client is shutting down, Completed when downloaded
    // completes Maybe<Event> event; Maybe<std::string> ip; Maybe<std::int16_t> numwant;
    //
    //

    std::string toHttpGetUrl() const;

    friend std::ostream &operator<<(std::ostream &, const TrackerRequest &tr);

    bool operator==(const TrackerRequest &tr) const;
};

/**
Expected response from tracker
*/
struct TrackerResponse
{
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

    friend std::ostream &operator<<(std::ostream &out, const TrackerResponse &s);

    bool operator==(const TrackerResponse &tr) const;

    static std::variant<std::string, TrackerResponse> decode(const bencode::bdict &bd);

    Announce toAnnounce(const common::InfoHash &, time_t now) const;
};
} // namespace fractals::network::http