#include "neither/maybe.hpp"
#include <fractals/network/http/Tracker.h>
#include <fractals/torrent/MetaInfo.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <optional>
#include <string>

using ::testing::_;

TEST(TRACKER, make_tracker_request_empty)
{
    const fractals::torrent::SingleFile sf
    { .name = neither::none
    , .length = 0
    , .md5sum = neither::none
    };

    const fractals::torrent::InfoDict info
    { .piece_length = 0
    , .pieces = std::vector<char>()
    , .publish = neither::none
    , .file_mode = neither::left(sf)
    };

    const fractals::torrent::MetaInfo mi
    { .announce = ""
    , .announce_list = neither::none
    , .creation_date = neither::none
    , .comment = neither::none
    , .encoding = neither::none
    , .info = info
    };

    auto res = fractals::network::http::makeTrackerRequest(mi);

    const fractals::network::http::TrackerRequest tr
    { .announce = ""
    , .info_hash = std::vector<char>()
    , .url_info_hash = ""
    , .peer_id = std::vector<char>()
    , .url_peer_id = ""
    , .port = 0
    , .uploaded = 0
    , .downloaded = 0
    , .left = 0
    , . compact = 0
    };

    ASSERT_EQ(tr, res);
}