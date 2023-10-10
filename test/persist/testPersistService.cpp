#include "fractals/persist/Event.h"
#include "fractals/persist/Models.h"
#include "fractals/persist/PersistEventQueue.h"
#include "fractals/persist/PersistService.ipp"
#include "fractals/sync/QueueCoordinator.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace fractals::persist
{

inline bool operator==(const TorrentModel &lhs, const TorrentModel &rhs)
{
    return lhs.id == rhs.id && lhs.name == rhs.name && lhs.meta_info_path == rhs.meta_info_path &&
           lhs.write_path == rhs.write_path;
}

inline bool operator==(const PieceModel &lhs, const PieceModel &rhs)
{
    return lhs.id == rhs.id && lhs.torrent_id == rhs.torrent_id && lhs.piece == rhs.piece;
}

inline bool operator==(const AnnounceModel &lhs, const AnnounceModel &rhs)
{
    return lhs.id == rhs.id && lhs.torrent_id == rhs.torrent_id && lhs.peer_ip == rhs.peer_ip &&
           lhs.peer_port == rhs.peer_port && lhs.announce_time == rhs.announce_time && lhs.interval == rhs.interval &&
           lhs.min_interval == rhs.min_interval;
}

class MockPersistClient
{
  public:
    MOCK_METHOD(void, addTorrent, (const AddTorrent &));
    MOCK_METHOD(std::optional<TorrentModel>, loadTorrent, (const std::string &));
    MOCK_METHOD(std::vector<TorrentModel>, loadTorrents, ());
    MOCK_METHOD(void, deleteTorrent, (const std::string &));

    MOCK_METHOD(void, addPiece, (const PieceComplete &));
    MOCK_METHOD(std::vector<PieceModel>, loadPieces, (const std::string &));
    MOCK_METHOD(void, deletePieces, (const std::string &));

    MOCK_METHOD(void, addAnnounce, (const AddAnnounce &));
    MOCK_METHOD(void, deleteAnnounce, (const std::string &));
    MOCK_METHOD(std::vector<AnnounceModel>, loadAnnounces, (const std::string &));
};

class PersistServiceTest : public ::testing::Test
{
  public:
    ::testing::StrictMock<MockPersistClient> client;
    PersistEventQueue queue;
    sync::QueueCoordinator coordinator;
    PersistServiceImpl<MockPersistClient> service{coordinator, queue.getRightEnd(), client};
};

TEST_F(PersistServiceTest, torrent)
{
    auto inQueue = queue.getLeftEnd();

    inQueue.push(LoadTorrent{"a"});
    inQueue.push(AddTorrent{"ih", "name", "torr", "out"});
    inQueue.push(LoadTorrents{});
    inQueue.push(RemoveTorrent{"a"});

    const auto tm = TorrentModel{0, "name", "path1", "path2"};
    const std::vector<TorrentModel> tms{tm};
    EXPECT_CALL(client, loadTorrent(_)).Times(1).WillOnce(Return(tm));
    EXPECT_CALL(client, addTorrent(_)).Times(1);
    EXPECT_CALL(client, loadTorrents()).Times(1).WillOnce(Return(tms));
    EXPECT_CALL(client, deleteTorrent(_)).Times(1);

    service.process();
    service.process();
    service.process();
    service.process();

    ASSERT_EQ(inQueue.numToRead(), 2);

    const auto res1 = inQueue.pop();
    ASSERT_EQ(tm, std::get<Torrent>(res1).result);
    const auto res2 = inQueue.pop();
    ASSERT_EQ(tms.size(), std::get<AllTorrents>(res2).result.size());
    ASSERT_EQ(tms.front(), std::get<AllTorrents>(res2).result.front());
}

TEST_F(PersistServiceTest, piece)
{
    auto inQueue = queue.getLeftEnd();

    inQueue.push(PieceComplete{"a", 0});
    inQueue.push(RemovePieces{"ih"});
    inQueue.push(LoadPieces{"a"});

    const auto pieces = {PieceModel{0, 0, 0}, PieceModel{1, 0, 1}, PieceModel{2, 0, 3}};
    EXPECT_CALL(client, addPiece(_)).Times(1);
    EXPECT_CALL(client, deletePieces(_)).Times(1);
    EXPECT_CALL(client, loadPieces(_)).Times(1).WillOnce(Return(pieces));

    service.process();
    service.process();
    service.process();

    ASSERT_EQ(inQueue.numToRead(), 1);

    const auto res1 = inQueue.pop();
    EXPECT_THAT(pieces, ContainerEq(std::get<Pieces>(res1).result));
}

TEST_F(PersistServiceTest, announce)
{
    auto inQueue = queue.getLeftEnd();

    const auto announces = {AnnounceModel{0, 0, "ip1", 1000, 50, 10, 10}, AnnounceModel{1, 1, "ip3", 99, 30, 20, 20},
                            AnnounceModel{2, 0, "ip2", 1001, 50, 10, 10}};
    inQueue.push(AddAnnounce{"a", "ip", 1600, 10, 1, std::nullopt});
    inQueue.push(RemoveAnnounces{"ih"});
    inQueue.push(LoadAnnounces{"a"});

    EXPECT_CALL(client, addAnnounce(_)).Times(1);
    EXPECT_CALL(client, deleteAnnounce(_)).Times(1);
    EXPECT_CALL(client, loadAnnounces(_)).Times(1).WillOnce(Return(announces));

    service.process();
    service.process();
    service.process();

    ASSERT_EQ(inQueue.numToRead(), 1);

    const auto res1 = inQueue.pop();
    EXPECT_THAT(announces, ContainerEq(std::get<Announces>(res1).result));
}

} // namespace fractals::persist