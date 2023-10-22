#include "fractals/common/Tagged.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/Models.h"
#include "fractals/persist/PersistEventQueue.h"
#include "fractals/persist/PersistService.ipp"
#include "fractals/sync/QueueCoordinator.h"
#include "fractals/torrent/MetaInfo.h"
#include "fractals/torrent/TorrentMeta.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <variant>

using namespace ::testing;

namespace fractals::persist
{

inline bool operator==(const TorrentModel &lhs, const TorrentModel &rhs)
{
    return lhs.id == rhs.id && lhs.name == rhs.name && lhs.metaInfoPath == rhs.metaInfoPath &&
           lhs.writePath == rhs.writePath;
}

inline bool operator==(const PieceModel &lhs, const PieceModel &rhs)
{
    return lhs.id == rhs.id && lhs.torrentId == rhs.torrentId && lhs.piece == rhs.piece;
}

inline bool operator==(const AnnounceModel &lhs, const AnnounceModel &rhs)
{
    return lhs.id == rhs.id && lhs.torrentId == rhs.torrentId && lhs.peerIp == rhs.peerIp &&
           lhs.peerPort == rhs.peerPort && lhs.announceTime == rhs.announceTime &&
           lhs.interval == rhs.interval && lhs.minInterval == rhs.minInterval;
}

class MockPersistClient
{
  public:
    MOCK_METHOD(void, openConnection, (const std::string &connString));

    using Return = std::variant<AddedTorrent, TorrentExists>;
    MOCK_METHOD(Return, addTorrent, (const AddTorrent &));
    MOCK_METHOD(std::optional<TorrentModel>, loadTorrent, (const common::InfoHash &));
    using ReturnLoadTorrents = std::vector<std::pair<TorrentModel, std::vector<FileModel>>>;
    MOCK_METHOD(ReturnLoadTorrents, loadTorrents, ());
    MOCK_METHOD(void, deleteTorrent, (const common::InfoHash &));

    MOCK_METHOD(void, addPiece, (const PieceComplete &));
    MOCK_METHOD(std::vector<PieceModel>, loadPieces, (const common::InfoHash &));
    MOCK_METHOD(void, deletePieces, (const common::InfoHash &));

    MOCK_METHOD(void, addAnnounce, (const AddAnnounce &));
    MOCK_METHOD(void, deleteAnnounce, (const common::InfoHash &));
    MOCK_METHOD(std::vector<AnnounceModel>, loadAnnounces, (const common::InfoHash &));

    MOCK_METHOD(void, addTrackers, (const AddTrackers &));
    MOCK_METHOD(std::vector<TrackerModel>, loadTrackers, (const common::InfoHash &));

    MOCK_METHOD(TorrentStats, loadTorrentStats, (const common::InfoHash &infoHash));
};

class PersistServiceTest : public ::testing::Test
{
  public:
    torrent::TorrentMeta TORRENT_META;
    common::InfoHash INFO_HASH;
    PersistServiceTest()
    {
        torrent::MetaInfo metaInfo;
        metaInfo.info.fileMode =
            torrent::MultiFile{"dirName",
                               {torrent::FileInfo{10, {}, {"dir1", "dir2", "file1.txt"}},
                                torrent::FileInfo{20, {}, {"abc", "deff", "file2.txt"}},
                                torrent::FileInfo{35, {}, {"ghjk", "file3.txt"}}}};
        TORRENT_META = torrent::TorrentMeta{metaInfo, "torrentName"};
        INFO_HASH = TORRENT_META.getInfoHash();
    }

  public:
    ::testing::NiceMock<MockPersistClient> client;
    PersistEventQueue queue;
    PersistEventQueue::LeftEndPoint btReqQueue = queue.getLeftEnd();
    AppPersistQueue appQueue;
    AppPersistQueue::LeftEndPoint appReqQueue = appQueue.getLeftEnd();
    sync::QueueCoordinator coordinator;
    PersistServiceImpl<MockPersistClient> service{coordinator, queue.getRightEnd(),
                                                  appQueue.getRightEnd(), client};
};

TEST_F(PersistServiceTest, torrent)
{
    btReqQueue.push(AddTorrent{TORRENT_META, "filepath", "writepath"});
    appReqQueue.push(LoadTorrents{});
    btReqQueue.push(RemoveTorrent{INFO_HASH});

    const auto tm = std::make_pair<TorrentModel, std::vector<FileModel>>(
        TorrentModel{0, "name", "path1", "path2"}, {});
    const std::vector<std::pair<TorrentModel, std::vector<FileModel>>> tms{tm};
    EXPECT_CALL(client, addTorrent(_)).Times(1).WillOnce(Return(AddedTorrent{tm.first, {}}));
    EXPECT_CALL(client, loadTorrents()).Times(1).WillOnce(Return(tms));
    EXPECT_CALL(client, deleteTorrent(_)).Times(1);

    service.processBtEvent();
    service.processAppEvent();
    service.processBtEvent();

    ASSERT_EQ(btReqQueue.numToRead(), 1);
    const auto res1 = btReqQueue.pop();
    ASSERT_TRUE(std::holds_alternative<AddedTorrent>(res1));
    ASSERT_EQ(tms.front().first, std::get<AddedTorrent>(res1).torrent);

    ASSERT_EQ(appReqQueue.numToRead(), 1);
    const auto res2 = appReqQueue.pop();
    ASSERT_EQ(tms.size(), std::get<AllTorrents>(res2).result.size());
    ASSERT_EQ(tms.front().first, std::get<AllTorrents>(res2).result.front().first);
}

TEST_F(PersistServiceTest, piece)
{
    btReqQueue.push(PieceComplete{INFO_HASH, 0});
    btReqQueue.push(RemovePieces{INFO_HASH});
    btReqQueue.push(LoadPieces{INFO_HASH});

    const auto pieces = {PieceModel{0, 0, 0}, PieceModel{1, 0, 1}, PieceModel{2, 0, 3}};
    EXPECT_CALL(client, addPiece(_)).Times(1);
    EXPECT_CALL(client, deletePieces(_)).Times(1);
    EXPECT_CALL(client, loadPieces(_)).Times(1).WillOnce(Return(pieces));

    service.processBtEvent();
    service.processBtEvent();
    service.processBtEvent();

    ASSERT_EQ(btReqQueue.numToRead(), 1);

    const auto res1 = btReqQueue.pop();
    EXPECT_THAT(pieces, ContainerEq(std::get<Pieces>(res1).result));
}

TEST_F(PersistServiceTest, announce)
{
    const auto announces = {AnnounceModel{0, 0, "ip1", 1000, 50, 10, 10},
                            AnnounceModel{1, 1, "ip3", 99, 30, 20, 20},
                            AnnounceModel{2, 0, "ip2", 1001, 50, 10, 10}};
    btReqQueue.push(AddAnnounce{INFO_HASH, "ip", 1600, 10, 1, std::nullopt});
    btReqQueue.push(RemoveAnnounces{INFO_HASH});
    btReqQueue.push(LoadAnnounces{INFO_HASH});

    EXPECT_CALL(client, addAnnounce(_)).Times(1);
    EXPECT_CALL(client, deleteAnnounce(_)).Times(1);
    EXPECT_CALL(client, loadAnnounces(_)).Times(1).WillOnce(Return(announces));

    service.processBtEvent();
    service.processBtEvent();
    service.processBtEvent();

    ASSERT_EQ(btReqQueue.numToRead(), 1);

    const auto res1 = btReqQueue.pop();
    EXPECT_THAT(announces, ContainerEq(std::get<Announces>(res1).result));
}

} // namespace fractals::persist