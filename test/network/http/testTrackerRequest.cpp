#include "Path.hpp"

#include "fractals/app/Client.h"
#include "fractals/common/Tagged.h"
#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/torrent/Bencode.h"
#include "fractals/torrent/TorrentMeta.h"
#include <fractals/network/http/TrackerClient.h>
#include <fractals/torrent/MetaInfo.h>

#include <bencode/decode.h>

#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neither/maybe.hpp>
#include <optional>
#include <stdexcept>
#include <string>

namespace fractals::network::http
{

class TrackerRequestTest : public ::testing::Test
{
  public:
    torrent::MetaInfo readTorrentFile()
    {
        std::fstream fs(test::TestData::makePath("dummy_torrent/MyTorrent.torrent"),
                        std::ios::binary | std::ios::in);

        if (!fs.is_open())
        {
            throw std::invalid_argument("Invalid file path");
        }

        std::stringstream buffer;
        buffer << fs.rdbuf();

        const auto mi =
            torrent::from_bdata<torrent::MetaInfo>(bencode::decode<bdata>(buffer).value());

        fs.close();

        return mi.rightValue;
    }
};

TEST_F(TrackerRequestTest, make_tracker_request_empty)
{
    const torrent::SingleFile sf{.name = "", .length = 0, .md5sum = {}};
    const torrent::InfoDict info{
        .piece_length = 0, .pieces = std::vector<char>(), .publish = std::nullopt, .file_mode = sf};
    const torrent::MetaInfo mi{.announce = "announce.url",
                               .announce_list = {},
                               .creation_date = std::nullopt,
                               .comment = std::nullopt,
                               .encoding = std::nullopt,
                               .info = info};

    const TrackerRequest req("announce.url", mi, app::generate_peerId());

    std::string bencoded("d6:lengthi0e4:name0:12:piece lengthi0e6:pieces0:e");
    const auto sha1 = common::sha1_encode<20>(bencoded);
    std::string_view vw{sha1.begin(), sha1.end()};
    common::InfoHash infoHash{sha1};
    const TrackerRequest tr("announce.url", infoHash, common::url_encode<20>(infoHash.underlying),
                            req.peer_id, common::url_encode<20>(req.peer_id), 6882, 0, 0, 0, 0);

    ASSERT_EQ(tr, req);
}

TEST_F(TrackerRequestTest, match_torrent_file)
{
    const auto APPID = app::generate_peerId();
    const auto sourceMetaInfo = readTorrentFile();
    const TrackerRequest sourceRequest("https://nyaa.si/", sourceMetaInfo, APPID);

    const torrent::FileInfo f1{.length = 60, .md5sum = {}, .path = {"dir1", "dir2", "test2 3.txt"}};
    const torrent::FileInfo f2{.length = 35, .md5sum = {}, .path = {"dir1", "dir2", "test2.txt"}};
    const torrent::FileInfo f3{.length = 12, .md5sum = {}, .path = {"dir1", "test2.txt"}};
    const torrent::FileInfo f4{.length = 4, .md5sum = {}, .path = {"test1.txt"}};

    const torrent::MultiFile mf{.name = "DummyTorrent"s, .files = {f1, f2, f3, f4}};
    const torrent::InfoDict info{
        .piece_length = 32768, .pieces = sourceMetaInfo.info.pieces, .publish = 0, .file_mode = mf};
    const std::vector<std::string> announceList{"https://nyaa.si/", "https://opentrackr.org/"};
    const torrent::MetaInfo mi{.announce = "https://nyaa.si/",
                               .announce_list = {{announceList}},
                               .creation_date = 1677099753,
                               .comment = {"This is a test torrent"},
                               .created_by = {"Ferdinand"},
                               .encoding = {"UTF-8"},
                               .info = info};

    TrackerRequest req("https://nyaa.si/", mi, APPID);
    req.peer_id = sourceRequest.peer_id;
    req.url_peer_id = sourceRequest.url_peer_id;

    ASSERT_EQ(sourceMetaInfo, mi);
    ASSERT_EQ(sourceRequest, req);
}

} // namespace fractals::network::http