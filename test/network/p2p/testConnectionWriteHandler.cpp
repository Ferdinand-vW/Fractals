#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollService.h"
#include "fractals/network/p2p/EpollService.ipp"
#include "fractals/network/p2p/EpollServiceEvent.h"
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
#include <fractals/torrent/MetaInfo.h>

#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <optional>
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
    MockBufferedQueueManager()
    {
    }

    void addToWriteBuffer(const PeerFd &p, std::vector<char> m)
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
    }

    bool addToReadBuffer(const PeerFd& p, std::vector<char> data)
    {
        return true;
    }

    void removeFromWriteBuffer(const PeerFd& p)
    {
        mBuffers.erase(p);
    }

    std::deque<ReadMsgState>& getReadBuffers(const PeerFd& p)
    {
        return readBuffers;
    }

    WriteMsgState *getWriteBuffer(const PeerFd& p)
    {
        return &mBuffers.at(p);
    }

    std::unordered_map<PeerFd, WriteMsgState> mBuffers;
    std::deque<ReadMsgState> readBuffers;
};

using MockWriteHandler = EpollServiceImpl<PeerFd, epoll_wrapper::Epoll<PeerFd>,
                                          MockBufferedQueueManager, EpollMsgQueue>;

std::vector<char> readFromFd(int fd)
{
    char c[512];
    int n = read(fd, &c[0], 512);
    assert(n > 0);

    return std::vector<char>(std::begin(c), std::begin(c) + n);
}

void writeToFd(int fd, std::string text)
{
    write(fd, text.c_str(), text.size());
}

std::pair<int, PeerFd> createPeer()
{
    int pipeFds[2];
    int pipeRes = pipe(pipeFds);

    assert(pipeRes == 0);

    PeerId pId("host", pipeFds[0]);
    return {pipeFds[0], PeerFd{pId, pipeFds[1]}};
}

TEST(CONNECTION_WRITE, sub_and_unsub)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    MockBufferedQueueManager bqm;
    EpollMsgQueue epollQueue;
    MockWriteHandler mw(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto t = std::thread([&]() { mw.run(); });

    auto queue = epollQueue.getLeftEnd();
    auto [writeFd1, peer1] = createPeer();
    bqm.addToWriteBuffer(peer1, {});
    queue.push(Subscribe{peer1});
    mw.notify();

    auto [writeFd2, peer2] = createPeer();
    bqm.addToWriteBuffer(peer2, {});
    queue.push(Subscribe{peer2});
    mw.notify();

    queue.push(UnSubscribe{peer1});
    mw.notify();

    queue.push(UnSubscribe{peer2});
    mw.notify();

    queue.push(Deactivate{});
    mw.notify();

    t.join();

    ASSERT_EQ(queue.numToRead(), 6);
    CtlResponse peer1Resp{peer1, ""};
    ConnectionCloseEvent peer1Close{peer1.getId()};
    CtlResponse peer2Resp{peer2, ""};
    ConnectionCloseEvent peer2Close{peer2.getId()};
    ASSERT_EQ(std::get<CtlResponse>(queue.pop()), peer1Resp);
    ASSERT_EQ(std::get<CtlResponse>(queue.pop()), peer2Resp);
    ASSERT_EQ(std::get<CtlResponse>(queue.pop()), peer1Resp);
    ASSERT_EQ(std::get<ConnectionCloseEvent>(queue.pop()), peer1Close);
    ASSERT_EQ(std::get<CtlResponse>(queue.pop()), peer2Resp);
    ASSERT_EQ(std::get<ConnectionCloseEvent>(queue.pop()), peer2Close);
    
    ASSERT_FALSE(mw.isActive());
}

TEST(CONNECTION_WRITE, one_subscriber_write_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    MockBufferedQueueManager bqm;
    EpollMsgQueue epollQueue;
    auto queue = epollQueue.getLeftEnd();
    MockWriteHandler mw(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto [readFd, peer] = createPeer();

    WriteEvent we{peer, {'t', 'e', 's', 't'}};

    auto t = std::thread([&]() { mw.run(); });

    queue.push(Subscribe{peer});
    queue.push(we);
    mw.notify();

    const auto receiveData = readFromFd(readFd);

    EXPECT_THAT(we.message, testing::ContainerEq(receiveData));

    queue.push(Deactivate{});
    mw.notify();

    t.join();
}

TEST(CONNECTION_WRITE, one_subscribed_write_multiple)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    EpollMsgQueue epollQueue;
    auto queue = epollQueue.getLeftEnd();
    MockBufferedQueueManager bqm;
    MockWriteHandler mw(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto [readFd, peer] = createPeer();

    auto t = std::thread([&]() { mw.run(); });

    {
        WriteEvent we{peer, {'t', 'e', 's', 't'}};

        queue.push(Subscribe(peer));
        queue.push(we);
        mw.notify();

        const auto receiveData = readFromFd(readFd);

        mw.notify();

        EXPECT_THAT(we.message, testing::ContainerEq(receiveData));
    }
    {
        WriteEvent we{peer, {'t', 'e', 's', 't', '1'}};

        queue.push(we);
        mw.notify();

        const auto receiveData = readFromFd(readFd);

        mw.notify();

        EXPECT_THAT(we.message, testing::ContainerEq(receiveData));
    }
    {
        WriteEvent we{peer, {'t', 'e', 's', 't', '2'}};

        queue.push(we);
        mw.notify();

        const auto receiveData = readFromFd(readFd);

        mw.notify();

        EXPECT_THAT(we.message, testing::ContainerEq(receiveData));
    }
    {
        WriteEvent we{peer, {'t', 'e', 's', 't', '4'}};

        queue.push(we);
        mw.notify();

        const auto receiveData = readFromFd(readFd);

        queue.push(UnSubscribe{peer});
        mw.notify();

        EXPECT_THAT(we.message, testing::ContainerEq(receiveData));
    }

    queue.push(Deactivate());
    mw.notify();

    t.join();
}

TEST(CONNECTION_WRITE, multiple_subscriber_write_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    EpollMsgQueue epollQueue;
    auto queue = epollQueue.getLeftEnd();
    MockBufferedQueueManager bqm;
    MockWriteHandler mw(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto [readFd1, peer1] = createPeer();
    auto [readFd2, peer2] = createPeer();
    auto [readFd3, peer3] = createPeer();

    auto t = std::thread([&]() { mw.run(); });


    WriteEvent we1{peer1, {'p', 'e', 'e', 'r', '1'}};
    WriteEvent we2{peer2, {'p', 'e', 'e', 'r', '1'}};
    WriteEvent we3{peer3, {'p', 'e', 'e', 'r', '1'}};
    queue.push(Subscribe{peer1});
    queue.push(Subscribe{peer2});
    queue.push(Subscribe{peer3});
    queue.push(we1);
    queue.push(we2);
    queue.push(we3);
    mw.notify();

    const auto peer1Receive = readFromFd(readFd1);
    const auto peer2Receive = readFromFd(readFd2);
    const auto peer3Receive = readFromFd(readFd3);

    EXPECT_THAT(we1.message, testing::ContainerEq(peer1Receive));
    EXPECT_THAT(we2.message, testing::ContainerEq(peer2Receive));
    EXPECT_THAT(we3.message, testing::ContainerEq(peer3Receive));

    queue.push(Deactivate{});
    mw.notify();

    t.join();
}
