#include "fractals/common/encode.h"
#include "fractals/torrent/Bencode.h"
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

    auto ih = fractals::common::sha1_encode(bencode::encode(fractals::torrent::to_bdict(info)));
    const fractals::network::http::TrackerRequest tr
    { .announce = ""
    , .info_hash = ih
    , .url_info_hash = fractals::common::url_encode(ih)
    , .peer_id = res.peer_id // automatically generated
    , .url_peer_id = fractals::common::url_encode(res.peer_id)
    , .port = 6882
    , .uploaded = 0
    , .downloaded = 0
    , .left = 0
    , .compact = 0
    };

    ASSERT_EQ(tr, res);
}