#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/EpollService.h"
#include "fractals/network/p2p/EpollService.ipp"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/PeerEventQueue.h"
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
using namespace fractals;


class MockBufferedQueueManager
{
    public:
        MockBufferedQueueManager(PeerEventQueue &queue) : mQueue(queue) {}

        void addToWriteBufferedQueueManager(const PeerFd& p, std::vector<char> m)
            {
                const auto it = mBuffers.find(p);
                if (it == mBuffers.end())
                {
                    mBuffers.emplace(p, WriteMsgState(std::move(m)));
                }
                else if (it->second.isComplete())
                {
                    mBuffers.at(p) = WriteMsgState(std::move(m));
                }

                notifyWriter(p);
            }

        void publishToQueue(PeerEvent)
        {

        }

        void setWriteNotifier(std::function<void(PeerFd)> notifyWriter)
        {
            this->notifyWriter = notifyWriter;
        }

        WriteMsgState* getWriteBuffer(const PeerFd p)
        {
            return &mBuffers.at(p);
        }

    std::unordered_map<PeerFd, WriteMsgState> mBuffers;
    PeerEventQueue &mQueue;

    std::function<void(PeerFd)> notifyWriter;  

};

using MockWriteHandler = EpollService<ActionType::WRITE, PeerFd, epoll_wrapper::Epoll<PeerFd>, MockBufferedQueueManager>;

std::vector<char> readFromFd(int fd)
{
    char c[512];
    int n = read(fd,&c[0],512);

    assert(n > 0);

    return std::vector<char>(std::begin(c), std::begin(c) + n);
}

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

    PeerId pId("host",pipeFds[0]);
    return {pipeFds[0], PeerFd{pId, Socket(pipeFds[1])}};
}

TEST(CONNECTION_WRITE, sub_and_unsub)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    PeerEventQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockWriteHandler mw(epoll.getEpoll(), bqm);

    auto t = std::thread([&](){
        mw.run();
    });

    auto [writeFd1, peer1] = createPeer();
    auto ctl1 = mw.subscribe(peer1);
    ASSERT_FALSE(ctl1.hasError());

    auto [writeFd2, peer2] = createPeer();
    auto ctl2 = mw.subscribe(peer2);
    ASSERT_FALSE(ctl2.hasError());

    auto ctl3 = mw.unsubscribe(peer1);
    ASSERT_FALSE(ctl3.hasError());

    auto ctl4 = mw.unsubscribe(peer2);
    ASSERT_FALSE(ctl4.hasError());

    mw.stop();

    t.join();

}

TEST(CONNECTION_WRITE, one_subscriber_write_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    PeerEventQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockWriteHandler mw(epoll.getEpoll(), bqm);
    
    auto [readFd, peer] = createPeer();

    const std::vector<char> writeData{'t','e','s','t'};

    bqm.addToWriteBufferedQueueManager(peer, writeData);

    auto t = std::thread([&](){
        mw.run();
    });

    // Give epoll thread enough time to wait
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const auto receiveData = readFromFd(readFd);

    EXPECT_THAT(writeData, testing::ContainerEq(receiveData));

    mw.stop();

    t.join();
}

TEST(CONNECTION_WRITE, one_subscribed_write_multiple)
{
       epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    PeerEventQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockWriteHandler mw(epoll.getEpoll(), bqm);
    
    auto [readFd, peer] = createPeer();

    auto t = std::thread([&](){
        mw.run();
    });

    {
        const std::vector<char> writeData{'t','e','s','t'};

        bqm.addToWriteBufferedQueueManager(peer, writeData);

        // Give epoll thread enough time to wait
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const auto receiveData = readFromFd(readFd);

        EXPECT_THAT(writeData, testing::ContainerEq(receiveData));

    }
    {

        const std::vector<char> writeData{'t','e','s','t','1'};

        bqm.addToWriteBufferedQueueManager(peer, writeData);

        // Give epoll thread enough time to wait
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const auto receiveData = readFromFd(readFd);

        EXPECT_THAT(writeData, testing::ContainerEq(receiveData));

    }
    {

        const std::vector<char> writeData{'t','e','s','t','2'};

        bqm.addToWriteBufferedQueueManager(peer, writeData);

        // Give epoll thread enough time to wait
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const auto receiveData = readFromFd(readFd);

        EXPECT_THAT(writeData, testing::ContainerEq(receiveData));

    }

    {
        const std::vector<char> writeData{'t','e','s','t','3'};

        bqm.addToWriteBufferedQueueManager(peer, writeData);

        // Give epoll thread enough time to wait
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        const auto receiveData = readFromFd(readFd);

        EXPECT_THAT(writeData, testing::ContainerEq(receiveData));

    }

    mw.stop();

    t.join();
}

TEST(CONNECTION_WRITE, multiple_subscriber_write_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    PeerEventQueue wq;
    MockBufferedQueueManager bqm(wq);
    MockWriteHandler mw(epoll.getEpoll(), bqm);

    auto [readFd1, peer1] = createPeer();
    auto [readFd2, peer2] = createPeer();
    auto [readFd3, peer3] = createPeer();

    
    auto t = std::thread([&](){
        mw.run();
    });

    // Give epoll thread enough time to wait
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const auto peer1Write = {'p','e','e','r','1'};
    const auto peer2Write = {'p','e','e','r','2'};
    const auto peer3Write = {'p','e','e','r','3'};
    bqm.addToWriteBufferedQueueManager(peer1, peer1Write);
    bqm.addToWriteBufferedQueueManager(peer2, peer2Write);
    bqm.addToWriteBufferedQueueManager(peer3, peer3Write);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));


    const auto peer1Receive = readFromFd(readFd1);
    const auto peer2Receive = readFromFd(readFd2);
    const auto peer3Receive = readFromFd(readFd3);

    EXPECT_THAT(peer1Write, testing::ContainerEq(peer1Receive));
    EXPECT_THAT(peer2Write, testing::ContainerEq(peer2Receive));
    EXPECT_THAT(peer3Write, testing::ContainerEq(peer3Receive));

    mw.stop();

    t.join();
}
