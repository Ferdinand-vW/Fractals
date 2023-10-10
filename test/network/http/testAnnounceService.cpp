#include "fractals/network/http/AnnounceService.ipp"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/sync/QueueCoordinator.h"

#include "gmock/gmock.h"
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

using namespace ::testing;

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
    ::testing::StrictMock<MockClient> mc;
    AnnounceEventQueue queue;
    sync::QueueCoordinator coordinator;

    AnnounceServiceImpl<MockClient> annService{coordinator, queue.getRightEnd(), mc};
};

TEST_F(AnnounceServiceTest, subscription)
{
    annService.subscribe("a");

    ASSERT_TRUE(annService.isSubscribed("a"));
    ASSERT_FALSE(annService.isSubscribed("b"));

    annService.subscribe("b");

    ASSERT_TRUE(annService.isSubscribed("a"));
    ASSERT_TRUE(annService.isSubscribed("b"));

    annService.unsubscribe("a");

    ASSERT_FALSE(annService.isSubscribed("a"));
    ASSERT_TRUE(annService.isSubscribed("b"));

    annService.unsubscribe("b");

    ASSERT_FALSE(annService.isSubscribed("a"));
    ASSERT_FALSE(annService.isSubscribed("b"));
}

TEST_F(AnnounceServiceTest, on_request)
{
    auto otherEnd = queue.getLeftEnd();

    EXPECT_CALL(mc, query(_, _)).Times(1);
    EXPECT_CALL(mc, poll())
        .WillOnce(Return(TrackerClient::PollResult{"a", TrackerResponse{}}))
        .WillOnce(Return(TrackerClient::PollResult{"", ""}));

    TrackerRequest req("announce", {}, {}, {}, {}, 0, 0, 0, 0, 0);
    otherEnd.push(std::move(req));

    annService.pollOnce();
    annService.pollOnce();

    // No subscribers
    ASSERT_EQ(otherEnd.numToRead(), 0);
}

TEST_F(AnnounceServiceTest, response_to_subscriber)
{
    auto otherEnd = queue.getLeftEnd();

    annService.subscribe("a");

    TrackerResponse resp{{"warn"}, 1, 2, {"abcde"}, 5, 4, {Peer{"Peer1",PeerId("ip1", 1)}}};
    EXPECT_CALL(mc, query(_, _)).Times(1);
    EXPECT_CALL(mc, poll())
        .WillOnce(Return(TrackerClient::PollResult{"a", resp}));

    TrackerRequest req("announce", {}, {}, {}, {}, 0, 0, 0, 0, 0);
    otherEnd.push(std::move(req));


    annService.pollOnce();

    ASSERT_EQ(otherEnd.numToRead(), 1);

    ASSERT_TRUE(otherEnd.canPop());
    const auto announce = otherEnd.pop();

    ASSERT_EQ(resp.toAnnounce(announce.announce_time), announce);
}

} // namespace fractals::network::http