#include "fractals/network/http/AnnounceService.ipp"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/AnnounceEventQueue.h"

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

TEST(MockClient, subscription)
{
    MockClient mc;
    AnnounceEventQueue queue;

    AnnounceServiceImpl<MockClient> annService{queue.getRightEnd(), mc};

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

TEST(MockClient, on_request)
{
    MockClient mc;
    AnnounceEventQueue queue;
    AnnounceServiceImpl<MockClient> annService{queue.getRightEnd(), mc};

    auto otherEnd = queue.getLeftEnd();

    EXPECT_CALL(mc, query(_, _)).Times(1);
    EXPECT_CALL(mc, poll())
        .WillOnce(Return(TrackerClient::PollResult{"a", TrackerResponse{}}))
        .WillOnce(Return(TrackerClient::PollResult{"", ""}));

    TrackerRequest req("announce", {}, {}, {}, {}, 0, 0, 0, 0, 0);
    otherEnd.push(std::move(req));

    ASSERT_TRUE(annService.pollOnce());
    ASSERT_FALSE(annService.pollOnce());
}

TEST(MockClient, response_to_subscriber)
{
    MockClient mc;
    AnnounceEventQueue queue;
    AnnounceServiceImpl<MockClient> annService{queue.getRightEnd(), mc};
    auto otherEnd = queue.getLeftEnd();

    annService.subscribe("a");

    TrackerResponse resp{{"warn"}, 1, 2, {"abcde"}, 5, 4, {Peer{"Peer1",PeerId("ip1", 1)}}};
    EXPECT_CALL(mc, query(_, _)).Times(1);
    EXPECT_CALL(mc, poll())
        .WillOnce(Return(TrackerClient::PollResult{"a", resp}));

    TrackerRequest req("announce", {}, {}, {}, {}, 0, 0, 0, 0, 0);
    otherEnd.push(std::move(req));


    ASSERT_TRUE(annService.pollOnce());

    ASSERT_TRUE(otherEnd.canPop());
    const auto announce = otherEnd.pop();

    ASSERT_EQ(resp.toAnnounce(announce.announce_time), announce);
}

} // namespace fractals::network::http