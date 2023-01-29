#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/ConnectionEventHandler.h"
#include "fractals/network/p2p/ConnectionEventHandler.ipp"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/torrent/Bencode.h"
#include "neither/maybe.hpp"
#include "gmock/gmock.h"
#include <boost/mp11/algorithm.hpp>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/EpollImpl.ipp>
#include <epoll_wrapper/Event.h>
#include <fractals/network/http/Tracker.h>
#include <fractals/torrent/MetaInfo.h>

#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <deque>
#include <fcntl.h>
#include <iterator>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace fractals::network::p2p;
using namespace fractals::network::http;




struct MockEvent
{
    std::vector<char> mData;
};

using MockQueue = WorkQueueImpl<128, MockEvent>;

class MockBufferedQueueManager
{
    public:
        MockBufferedQueueManager(MockQueue &queue) : mQueue(queue) {}

        void addToReadBuffers(const PeerId& p, fractals::common::string_view data)
        {
            mQueue.push(MockEvent{std::vector<char>(data.begin(), data.end())});
        }

        void publishToQueue(PeerEvent)
        {

        }

        void setWriteNotifier(std::function<void(PeerFd)> notifyWriter)
        {
            // this->notifyWriter = notifyWriter;
        }

    MockQueue &mQueue;
};

using MockReadHandler = ConnectionEventHandler<ActionType::READ, PeerFd, epoll_wrapper::Epoll<PeerFd>, MockBufferedQueueManager>;


void writeToFd(int fd, std::string text)
{
    write (fd, text.c_str(), text.size());
}

std::pair<int, PeerFd> createPeer()
{
    int pipeFds[2];
    int pipeRes = pipe(pipeFds);

    // Set file descriptor to non blocking
    fcntl(pipeFds[0], F_SETFL, O_NONBLOCK);

    assert(pipeRes == 0);

    PeerId pId("host",0);
    return {pipeFds[1], PeerFd{pId, Socket(pipeFds[0])}};
}

TEST(CONNECTION_READ, sub_and_unsub)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    MockQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockReadHandler mr(epoll.getEpoll(), bqm);

    auto t = std::thread([&](){
        mr.run();
    });

    auto [writeFd1, peer1] = createPeer();
    auto ctl1 = mr.subscribe(peer1);
    ASSERT_FALSE(ctl1.hasError());

    auto [writeFd2, peer2] = createPeer();
    auto ctl2 = mr.subscribe(peer2);
    ASSERT_FALSE(ctl2.hasError());

    auto ctl3 = mr.unsubscribe(peer1);
    ASSERT_FALSE(ctl3.hasError());

    auto ctl4 = mr.unsubscribe(peer2);
    ASSERT_FALSE(ctl4.hasError());

    mr.stop();

    t.join();

}

TEST(CONNECTION_READ, one_subscriber_read_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    MockQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockReadHandler mr(epoll.getEpoll(), bqm);

    auto [writeFd, peer] = createPeer();

    auto ctl = mr.subscribe(peer);

    ASSERT_FALSE(ctl.hasError());

    writeToFd(writeFd, "test");

    auto t = std::thread([&](){
        mr.run();
    });

    // Give epoll thread enough time to wait
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_TRUE(!wq.isEmpty());
    MockEvent me = wq.pop();

    ASSERT_TRUE(wq.isEmpty());

    EXPECT_THAT(me.mData, testing::ContainerEq<std::vector<char>>({'t','e','s','t'}));

    mr.stop();

    t.join();
}

TEST(CONNECTION_READ, one_subscribed_read_multiple)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    MockQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockReadHandler mr(epoll.getEpoll(), bqm);

    auto [writeFd, peer] = createPeer();

    auto ctl = mr.subscribe(peer);

    ASSERT_FALSE(ctl.hasError());

    writeToFd(writeFd, "test");

    auto t = std::thread([&](){
        mr.run();
    });

    // Give epoll thread enough time to wait
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    writeToFd(writeFd, "test1");

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    writeToFd(writeFd, "test2");

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    writeToFd(writeFd, "test3");

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_TRUE(!wq.isEmpty());
    ASSERT_EQ(wq.size(), 4);
    MockEvent me = wq.pop();
    MockEvent me2 = wq.pop();
    MockEvent me3 = wq.pop();
    MockEvent me4 = wq.pop();
    ASSERT_TRUE(wq.isEmpty());

    EXPECT_THAT(me.mData, testing::ContainerEq<std::vector<char>>({'t','e','s','t'}));

    EXPECT_THAT(me2.mData, testing::ContainerEq<std::vector<char>>({'t','e','s','t','1'}));

    EXPECT_THAT(me3.mData, testing::ContainerEq<std::vector<char>>({'t','e','s','t','2'}));

    EXPECT_THAT(me4.mData, testing::ContainerEq<std::vector<char>>({'t','e','s','t','3'}));

    mr.stop();

    t.join();
}

TEST(CONNECTION_READ, multiple_subscriber_read_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    MockQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockReadHandler mr(epoll.getEpoll(), bqm);

    auto [writeFd1, peer1] = createPeer();
    auto [writeFd2, peer2] = createPeer();
    auto [writeFd3, peer3] = createPeer();

    auto ctl1 = mr.subscribe(peer1);
    ASSERT_FALSE(ctl1.hasError());
    auto ctl2 = mr.subscribe(peer2);
    ASSERT_FALSE(ctl2.hasError());
    auto ctl3 = mr.subscribe(peer3);
    ASSERT_FALSE(ctl3.hasError());

    writeToFd(writeFd1, "peer1");
    writeToFd(writeFd2, "peer2");
    writeToFd(writeFd3, "peer3");

    auto t = std::thread([&](){
        mr.run();
    });

    // Give epoll thread enough time to wait
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_TRUE(!wq.isEmpty());
    ASSERT_EQ(wq.size(), 3);
    MockEvent me1 = wq.pop();
    MockEvent me2 = wq.pop();
    MockEvent me3 = wq.pop();
    ASSERT_TRUE(wq.isEmpty());

    EXPECT_THAT(me1.mData, testing::ContainerEq<std::vector<char>>({'p','e','e','r','1'}));
    EXPECT_THAT(me2.mData, testing::ContainerEq<std::vector<char>>({'p','e','e','r','2'}));
    EXPECT_THAT(me3.mData, testing::ContainerEq<std::vector<char>>({'p','e','e','r','3'}));

    mr.stop();

    t.join();
}

TEST(CONNECTION_READ, multiple_subscribed_read_multiple)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    MockQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockReadHandler mr(epoll.getEpoll(), bqm);

    auto t = std::thread([&](){
        mr.run();
    });

    constexpr int numPeers = 5;
    for (int p = 0; p < numPeers; p++)
    {
        auto [writeFd, peerFd] = createPeer();

        auto subCtl = mr.subscribe(peerFd);

        ASSERT_FALSE(subCtl.hasError());

        ASSERT_TRUE(mr.isSubscribed(peerFd));
        writeToFd(writeFd, "msg received");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(40));

    ASSERT_TRUE(wq.size() >= numPeers);

    for (int p = 0; p < numPeers; p++)
    {
        MockEvent me = wq.pop();
        ASSERT_EQ(fractals::common::ascii_decode(me.mData), "msg received");
    }

    mr.stop();

    t.join();
}