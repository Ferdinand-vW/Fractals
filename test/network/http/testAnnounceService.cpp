#include "fractals/common/Tagged.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/network/http/AnnounceService.ipp"
#include "fractals/network/http/Event.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerClient.h"
#include "fractals/persist/Models.h"
#include "fractals/sync/QueueCoordinator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <variant>

using namespace ::testing;
using namespace std::string_literals;

namespace fractals::network::http
{

class MockClient
{
  public:
    MOCK_METHOD(void, query, (const TrackerRequest &, std::chrono::milliseconds));
    MOCK_METHOD(TrackerClient::PollResult, poll, ());
};

class AnnounceServiceTest : public ::testing::Test
{
  public:
    static constexpr common::InfoHash INFO_HASH{"abcdefg"};
    static constexpr http::Peer PEER_A{"peerA", http::PeerId{"ipAddressA", 1000}};
    static constexpr http::Peer PEER_B{"peerB", http::PeerId{"ipAddressB", 1001}};

    AnnounceServiceTest()
    {
        ON_CALL(mc, poll()).WillByDefault(Return(TrackerClient::PollResult{}));
        Fractals::initAppId();
    }

    ::testing::NiceMock<MockClient> mc;
    AnnounceEventQueue queue;
    AnnounceEventQueue::LeftEndPoint requestQueue = queue.getLeftEnd();
    sync::QueueCoordinator coordinator;

    AnnounceServiceImpl<MockClient> annService{coordinator, queue.getRightEnd(), mc};
};

TEST_F(AnnounceServiceTest, OnRequestAnnounceNoTrackers)
{
    requestQueue.push(RequestAnnounce{INFO_HASH});

    EXPECT_CALL(mc, query(_, _)).Times(0);
    annService.pollOnce();
}

TEST_F(AnnounceServiceTest, OnRequestAnnounceExecute)
{
    persist::TorrentModel torrModel;
    requestQueue.push(AddTrackers{INFO_HASH, {persist::TrackerModel{0, 0, "announce.url"}}});
    requestQueue.push(RequestAnnounce{INFO_HASH, torrModel});

    EXPECT_CALL(mc, query(TrackerRequest{"announce.url", torrModel, Fractals::APPID}, _)).Times(1);
    annService.pollOnce();
    annService.pollOnce();
}

TEST_F(AnnounceServiceTest, OnRequestAnnounceDelayed)
{
    requestQueue.push(RequestAnnounce{INFO_HASH});

    EXPECT_CALL(mc, query(_, _)).Times(0);
    annService.pollOnce();

    requestQueue.push(AddTrackers{INFO_HASH, {persist::TrackerModel{0, 0, "announce.url"}}});
    EXPECT_CALL(mc,
                query(TrackerRequest{"announce.url", persist::TorrentModel{}, Fractals::APPID}, _))
        .Times(1);
    annService.pollOnce();
}

TEST_F(AnnounceServiceTest, OnRequestAnnounceAlreadySubscribed)
{
    persist::TorrentModel torrModel;
    requestQueue.push(AddTrackers{INFO_HASH, {persist::TrackerModel{0, 0, "announce.url"}}});
    requestQueue.push(RequestAnnounce{INFO_HASH, torrModel});
    requestQueue.push(RequestAnnounce{INFO_HASH, torrModel});

    EXPECT_CALL(mc, query(TrackerRequest{"announce.url", torrModel, Fractals::APPID}, _)).Times(1);
    annService.pollOnce();
    annService.pollOnce();
    EXPECT_CALL(mc, query(_, _)).Times(0);
    annService.pollOnce();
}

TEST_F(AnnounceServiceTest, PollResponseSuccess)
{
    { // setup
        requestQueue.push(AddTrackers{INFO_HASH, {persist::TrackerModel{0, 0, "announce.url"}}});
        annService.pollOnce();
    }

    persist::TorrentModel torrModel;
    requestQueue.push(RequestAnnounce{INFO_HASH, torrModel});

    EXPECT_CALL(mc, query(TrackerRequest{"announce.url", torrModel, Fractals::APPID}, _)).Times(1);
    TrackerResponse trackerResponse{};
    trackerResponse.peers = {PEER_A};
    const TrackerClient::PollResult pollResult{INFO_HASH, "announce.url", trackerResponse};
    EXPECT_CALL(mc, poll()).WillOnce(Return(pollResult));
    annService.pollOnce();

    ASSERT_TRUE(requestQueue.numToRead());
    const Announce msg = requestQueue.pop();
    ASSERT_EQ(msg.infoHash, INFO_HASH);
    ASSERT_EQ(msg.peers.size(), 1);
    ASSERT_EQ(msg.peers.front(), PEER_A.id);
}

TEST_F(AnnounceServiceTest, PollResponseChanges)
{
    { // setup
        requestQueue.push(AddTrackers{INFO_HASH, {persist::TrackerModel{0, 0, "announce.url"}}});
        annService.pollOnce();
    }

    persist::TorrentModel torrModel;

    { // First query
        requestQueue.push(RequestAnnounce{INFO_HASH, torrModel});

        EXPECT_CALL(mc, query(TrackerRequest{"announce.url", torrModel, Fractals::APPID}, _))
            .Times(1);
        TrackerResponse trackerResponse{};
        trackerResponse.peers = {PEER_A};
        const TrackerClient::PollResult pollResult{INFO_HASH, "announce.url", trackerResponse};
        EXPECT_CALL(mc, poll()).WillOnce(Return(pollResult));
        annService.pollOnce();

        ASSERT_TRUE(requestQueue.numToRead());
        const Announce msg = requestQueue.pop();
        ASSERT_EQ(msg.infoHash, INFO_HASH);
        ASSERT_EQ(msg.peers.size(), 1);
        ASSERT_EQ(msg.peers.front(), PEER_A.id);
    }

    { // Second query, exact same request and response
        requestQueue.push(RequestAnnounce{INFO_HASH, torrModel});

        EXPECT_CALL(mc, query(TrackerRequest{"announce.url", torrModel, Fractals::APPID}, _))
            .Times(1);
        TrackerResponse trackerResponse{};
        trackerResponse.peers = {PEER_A};
        const TrackerClient::PollResult pollResult{INFO_HASH, "announce.url", trackerResponse};
        EXPECT_CALL(mc, poll()).WillOnce(Return(pollResult));
        annService.pollOnce();

        // Nothing to report
        ASSERT_FALSE(requestQueue.numToRead());
    }

    { // Third query, exact same request and new peer
        requestQueue.push(RequestAnnounce{INFO_HASH, torrModel});

        EXPECT_CALL(mc, query(TrackerRequest{"announce.url", torrModel, Fractals::APPID}, _))
            .Times(1);
        TrackerResponse trackerResponse{};
        trackerResponse.peers = {PEER_A, PEER_B};
        const TrackerClient::PollResult pollResult{INFO_HASH, "announce.url", trackerResponse};
        EXPECT_CALL(mc, poll()).WillOnce(Return(pollResult));
        annService.pollOnce();

        // PEER_B is new so announce it
        ASSERT_TRUE(requestQueue.numToRead());
        const Announce msg = requestQueue.pop();
        ASSERT_EQ(msg.infoHash, INFO_HASH);
        ASSERT_EQ(msg.peers.size(), 1);
        ASSERT_EQ(msg.peers.front(), PEER_B.id);
    }
}

} // namespace fractals::network::http