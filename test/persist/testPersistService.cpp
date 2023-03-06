#include "fractals/persist/Event.h"
#include "fractals/persist/Models.h"
#include "fractals/persist/PersistService.ipp"
#include "fractals/persist/PersistEventQueue.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

namespace fractals::persist
{


class MockPersistClient
{
    public:

    MOCK_METHOD(void, addTorrent, (const AddTorrent&));
    MOCK_METHOD(std::optional<TorrentModel>, loadTorrent, (const std::string&));
    MOCK_METHOD(std::vector<TorrentModel>, loadTorrents, ());
    MOCK_METHOD(void, deleteTorrent, (const std::string&));

    MOCK_METHOD(void, addPiece, (const AddPiece&));
    MOCK_METHOD(std::vector<PieceModel>, loadPieces, (const std::string&));
    MOCK_METHOD(void, deletePieces, (const std::string&));

    MOCK_METHOD(void, addAnnounce, (const AddAnnounce&));
    MOCK_METHOD(void, deleteAnnounce, (const std::string&));
    MOCK_METHOD(std::vector<AnnounceModel>, loadAnnounces, (const std::string&));
};

TEST(PERSIST_SERVICE, torrent)
{
    MockPersistClient client;
    PersistEventQueue queue;
    PersistServiceImpl<MockPersistClient> service(queue.getRightEnd(), client);

    auto inQueue = queue.getLeftEnd();

    inQueue.push(LoadTorrent{"a"});
    inQueue.push(AddTorrent{"ih","name","torr","out"});
    inQueue.push(LoadTorrents{});
    inQueue.push(RemoveTorrent{"a"});

    EXPECT_CALL(client, loadTorrent(_)).Times(1);
    EXPECT_CALL(client, addTorrent(_)).Times(1);
    EXPECT_CALL(client, loadTorrents()).Times(1);
    EXPECT_CALL(client, deleteTorrent(_)).Times(1);

    service.pollOnce();
    service.pollOnce();
    service.pollOnce();
    service.pollOnce();
}

TEST(PERSIST_SERVICE, piece)
{
    MockPersistClient client;
    PersistEventQueue queue;
    PersistServiceImpl<MockPersistClient> service(queue.getRightEnd(), client);

    auto inQueue = queue.getLeftEnd();

    inQueue.push(AddPiece{"a",0});
    inQueue.push(RemovePieces{"ih"});
    inQueue.push(LoadPieces{"a"});

    EXPECT_CALL(client, addPiece(_)).Times(1);
    EXPECT_CALL(client, deletePieces(_)).Times(1);
    EXPECT_CALL(client, loadPieces(_)).Times(1);

    service.pollOnce();
    service.pollOnce();
    service.pollOnce();
}

TEST(PERSIST_SERVICE, announce)
{
    MockPersistClient client;
    PersistEventQueue queue;
    PersistServiceImpl<MockPersistClient> service(queue.getRightEnd(), client);

    auto inQueue = queue.getLeftEnd();

    inQueue.push(AddAnnounce{"a","ip", 1600, 10, 1, std::nullopt});
    inQueue.push(RemoveAnnounces{"ih"});
    inQueue.push(LoadAnnounces{"a"});

    EXPECT_CALL(client, addAnnounce(_)).Times(1);
    EXPECT_CALL(client, deleteAnnounce(_)).Times(1);
    EXPECT_CALL(client, loadAnnounces(_)).Times(1);

    service.pollOnce();
    service.pollOnce();
    service.pollOnce();
}

}