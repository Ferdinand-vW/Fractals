#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/torrent/Bencode.h"
#include "neither/maybe.hpp"
#include <fractals/network/http/TrackerClient.h>
#include <fractals/torrent/MetaInfo.h>

#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
#include <string>

namespace fractals::network::http
{

std::string getByteData()
{
    std::fstream fs("../../../../examples/ub.torrent", std::ios::binary | std::ios::in);

    if (!fs.is_open())
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
    const torrent::SingleFile sf{.name = neither::none, .length = 0, .md5sum = neither::none};
    const torrent::InfoDict info{
        .piece_length = 0, .pieces = std::vector<char>(), .publish = neither::none, .file_mode = neither::left(sf)};
    const torrent::MetaInfo mi{.announce = "",
                               .announce_list = neither::none,
                               .creation_date = neither::none,
                               .comment = neither::none,
                               .encoding = neither::none,
                               .info = info};

    const TrackerRequest req(mi);

    auto info_hash_hex = "a3e4ef2e3999f595f55c215ae0854c760960693b";
    auto info_hash_bytes = common::hex_to_bytes(info_hash_hex);
    const TrackerRequest tr("", info_hash_bytes, common::url_encode(info_hash_bytes), req.peer_id,
                            common::url_encode(req.peer_id), 6882, 0, 0, 0, 0);

    ASSERT_EQ(tr, req);
}

TEST(TRACKER, make_tracker_request_single_file)
{
    const torrent::SingleFile sf{.name = "make_tracker_request_single_file"s, .length = 10000, .md5sum = neither::none};
    const auto hexBytes = getByteData();
    const torrent::InfoDict info{
        .piece_length = 20, .pieces = hex_to_bytes(hexBytes), .publish = 1, .file_mode = neither::left(sf)};

    const torrent::MetaInfo mi{.announce = "CustomTracker",
                               .announce_list = neither::none,
                               .creation_date = neither::none,
                               .comment = neither::none,
                               .encoding = neither::none,
                               .info = info};

    const TrackerRequest req(mi);

    const auto info_hash_hex = "ecb15ee23a74c98cfd667dcd263ecc9721b478f6";
    const auto info_hash_bytes = common::hex_to_bytes(info_hash_hex);
    const TrackerRequest tr("CustomTracker", info_hash_bytes, common::url_encode(info_hash_bytes),
                            req.peer_id // automatically generated
                            ,
                            common::url_encode(req.peer_id), 6882, 0, 0, 0, 0);

    ASSERT_EQ(tr, req);
}

TEST(TRACKER, make_tracker_request_multi_file)
{
    const torrent::FileInfo f1{.length = 4000, .md5sum = neither::none, .path = {"dir1", "file1.txt"}};
    const torrent::FileInfo f2{.length = 2000, .md5sum = neither::none, .path = {"dir1", "file2.txt"}};
    const torrent::FileInfo f3{.length = 4000, .md5sum = neither::none, .path = {"dir2", "dir3", "file1.txt"}};
    const torrent::MultiFile mf{.name = "make_tracker_request_single_file"s, .files = {f1, f2, f3}};
    const auto hexBytes = getByteData();
    const torrent::InfoDict info{
        .piece_length = 20, .pieces = hex_to_bytes(hexBytes), .publish = 1, .file_mode = neither::right(mf)};
    const torrent::MetaInfo mi{.announce = "CustomTracker",
                               .announce_list = neither::none,
                               .creation_date = neither::none,
                               .comment = neither::none,
                               .encoding = neither::none,
                               .info = info};

    const TrackerRequest req(mi);

    const auto info_hash_hex = "1fe94a3ec02dfd496561f00a350d939bfa563df9";
    const auto info_hash_bytes = common::hex_to_bytes(info_hash_hex);
    const TrackerRequest tr("CustomTracker", info_hash_bytes, common::url_encode(info_hash_bytes), req.peer_id,
                            common::url_encode(req.peer_id), 6882, 0, 0, 0, 0);

    ASSERT_EQ(tr, req);
}

} // namespace fractals::network::http