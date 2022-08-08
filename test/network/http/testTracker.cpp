#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/torrent/Bencode.h"
#include "neither/maybe.hpp"
#include <fractals/network/http/Tracker.h>
#include <fractals/torrent/MetaInfo.h>

#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <optional>
#include <stdexcept>
#include <string>

using ::testing::_;

std::string getByteData()
{
    std::fstream fs("../../../../examples/10000_bytes.txt", ios::binary | ios::in);

    if(!fs.is_open())
    {
        throw std::invalid_argument("Invalid file path");
    }

    
    std::stringstream buffer;
    buffer << fs.rdbuf();

    fs.close();

    return buffer.str();
}

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

    auto info_hash_hex = "a3e4ef2e3999f595f55c215ae0854c760960693b";
    auto info_hash_bytes = fractals::common::hex_to_bytes(info_hash_hex);
    const fractals::network::http::TrackerRequest tr
    { .announce = ""
    , .info_hash = info_hash_bytes
    , .url_info_hash = fractals::common::url_encode(info_hash_bytes)
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

TEST(TRACKER, make_tracker_request_single_file)
{
    const fractals::torrent::SingleFile sf
    { .name = "make_tracker_request_single_file"s
    , .length = 10000
    , .md5sum = neither::none
    };

    auto hexBytes = getByteData();

    const fractals::torrent::InfoDict info
    { .piece_length = 20
    , .pieces = hex_to_bytes(hexBytes)
    , .publish = 1
    , .file_mode = neither::left(sf)
    };

    const fractals::torrent::MetaInfo mi
    { .announce = "CustomTracker"
    , .announce_list = neither::none
    , .creation_date = neither::none
    , .comment = neither::none
    , .encoding = neither::none
    , .info = info
    };

    auto res = fractals::network::http::makeTrackerRequest(mi);

    auto info_hash_hex = "ecb15ee23a74c98cfd667dcd263ecc9721b478f6";
    auto info_hash_bytes = fractals::common::hex_to_bytes(info_hash_hex);
    const fractals::network::http::TrackerRequest tr
    { .announce = "CustomTracker"
    , .info_hash = info_hash_bytes
    , .url_info_hash = fractals::common::url_encode(info_hash_bytes)
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

TEST(TRACKER, make_tracker_request_multi_file)
{
    const fractals::torrent::FileInfo f1
    { .length = 4000
    , .md5sum = neither::none
    , .path = {"dir1","file1.txt"}
    };

    const fractals::torrent::FileInfo f2
    { .length = 2000
    , .md5sum = neither::none
    , .path = {"dir1","file2.txt"}
    };

    const fractals::torrent::FileInfo f3
    { .length = 4000
    , .md5sum = neither::none
    , .path = {"dir2","dir3","file1.txt"}
    };

    const fractals::torrent::MultiFile mf
    { .name = "make_tracker_request_single_file"s
    , .files = {f1,f2,f3}
    };

    auto hexBytes = getByteData();

    const fractals::torrent::InfoDict info
    { .piece_length = 20
    , .pieces = hex_to_bytes(hexBytes)
    , .publish = 1
    , .file_mode = neither::right(mf)
    };

    const fractals::torrent::MetaInfo mi
    { .announce = "CustomTracker"
    , .announce_list = neither::none
    , .creation_date = neither::none
    , .comment = neither::none
    , .encoding = neither::none
    , .info = info
    };

    auto res = fractals::network::http::makeTrackerRequest(mi);

    auto info_hash_hex = "1fe94a3ec02dfd496561f00a350d939bfa563df9";
    auto info_hash_bytes = fractals::common::hex_to_bytes(info_hash_hex);
    const fractals::network::http::TrackerRequest tr
    { .announce = "CustomTracker"
    , .info_hash = info_hash_bytes
    , .url_info_hash = fractals::common::url_encode(info_hash_bytes)
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
