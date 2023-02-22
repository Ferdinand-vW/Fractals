#include "Path.hpp"

#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/torrent/Bencode.h"
#include "fractals/torrent/TorrentMeta.h"
#include <bencode/decode.h>
#include <fractals/network/http/TrackerClient.h>
#include <fractals/torrent/MetaInfo.h>

#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neither/maybe.hpp>
#include <optional>
#include <stdexcept>
#include <string>

namespace fractals::network::http
{

torrent::MetaInfo readTorrentFile()
{
    std::fstream fs(test::TestData::makePath("dummy_torrent/MyTorrent.torrent"), std::ios::binary | std::ios::in);

    if (!fs.is_open())
    {
        throw std::invalid_argument("Invalid file path");
    }

    std::stringstream buffer;
    buffer << fs.rdbuf();

    const auto mi = torrent::from_bdata<torrent::MetaInfo>(bencode::decode<bdata>(buffer).value());

    fs.close();

    return mi.rightValue;
}

TEST(TRACKER_REQUEST, make_tracker_request_empty)
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

TEST(TRACKER_REQUEST, match_torrent_file)
{
    const auto sourceMetaInfo = readTorrentFile();
    const TrackerRequest sourceRequest(sourceMetaInfo); 

    const torrent::FileInfo f1{.length = 60, .md5sum = neither::none, .path = {"dir1", "dir2", "test2 3.txt"}};
    const torrent::FileInfo f2{.length = 35, .md5sum = neither::none, .path = {"dir1", "dir2", "test2.txt"}};
    const torrent::FileInfo f3{.length = 12, .md5sum = neither::none, .path = {"dir1", "test2.txt"}};
    const torrent::FileInfo f4{.length = 4, .md5sum = neither::none, .path = {"test1.txt"}};
    
    const torrent::MultiFile mf{.name = "DummyTorrent"s, .files = {f1, f2, f3, f4}};
    const torrent::InfoDict info{
        .piece_length = 32768, .pieces = sourceMetaInfo.info.pieces, .publish = 0, .file_mode = neither::right(mf)};
    const std::vector<std::string> announceList{"https://nyaa.si/","https://opentrackr.org/"};
    const torrent::MetaInfo mi{.announce = "https://nyaa.si/",
                               .announce_list = {{announceList}},
                               .creation_date = 1677099753,
                               .comment = {"This is a test torrent"},
                               .created_by = {"Ferdinand"},
                               .encoding = {"UTF-8"},
                               .info = info};

    TrackerRequest req(mi);
    req.peer_id = sourceRequest.peer_id;
    req.url_peer_id = sourceRequest.url_peer_id;

    ASSERT_EQ(sourceMetaInfo, mi);
    ASSERT_EQ(sourceRequest, req);
}

} // namespace fractals::network::http