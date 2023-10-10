#pragma once

#include "fractals/common/Tagged.h"
#include "fractals/network/http/Request.h"
#include "fractals/persist/Models.h"
#include "fractals/torrent/MetaInfo.h"
namespace fractals::network::http
{
    struct AddTrackers
    {
        common::InfoHash infoHash;
        std::vector<persist::TrackerModel> trackers;
    };

    struct RequestAnnounce
    {
        common::InfoHash infoHash;
        persist::TorrentModel torrent;
    };

    struct DeleteTrackers
    {
        common::InfoHash infoHash;
    };

    struct Pause
    {
        common::InfoHash infoHash;
    };

    struct Shutdown
    {

    };

    using AnnounceRequest = std::variant<RequestAnnounce, AddTrackers, DeleteTrackers, Pause, Shutdown>;
    using AnnounceResponse = Announce;
}