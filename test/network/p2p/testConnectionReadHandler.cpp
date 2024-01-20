#include <fractals/common/encode.h>
#include <fractals/common/utils.h>
#include <fractals/network/http/Peer.h>
#include <fractals/network/http/Request.h>
#include <fractals/network/p2p/BufferedQueueManager.h>
#include <fractals/network/p2p/EpollMsgQueue.h>
#include <fractals/network/p2p/EpollService.h>
#include <fractals/network/p2p/EpollService.ipp>
#include <fractals/network/p2p/EpollServiceEvent.h>
#include <fractals/network/p2p/PeerFd.h>
#include <fractals/torrent/Bencode.h>
#include "neither/maybe.hpp"
#include <boost/mp11/algorithm.hpp>
#include <cassert>
#include <chrono>
#include <condition_variable>
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
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

#define ASSERT_READ(readEvent, v) EXPECT_THAT(readEvent.mMessage, v)

namespace fractals::network::p2p
{

class MockBufferedQueueManager
{
  public:
    MockBufferedQueueManager()
    {
    }

    bool addToReadBuffer(PeerFd peer, std::vector<char> buf)
    {
        bool firstByte = true;
        int s = 0;
        ReadMsgState m;

        for (int i = 0; i < buf.size(); ++i)
        {
            if (firstByte)
            {
                s = buf[i];
                m.initialize(s);
                firstByte = false;
            }
            else
            {
                m.append(std::string{buf[i]}, 1);
                s--;

                if (s == 0)
                {
                    bufMap[peer].push_back(m);
                    m = ReadMsgState{};
                    firstByte = true;
                }
            }
        }

        return true;
    }

    void addToWriteBuffer(const PeerFd &p, std::vector<char> &&m)
    {
    }

    std::deque<ReadMsgState> &getReadBuffers(PeerFd peer)
    {
        return bufMap[peer];
    }

    WriteMsgState *getWriteBuffer(PeerFd peer)
    {
        return nullptr;
    }

    void removeFromWriteBuffer(PeerFd peer)
    {
        bufMap.erase(peer);
    }

    std::unordered_map<PeerFd, std::deque<ReadMsgState>> bufMap;
};

using MockReadHandler =
    EpollServiceImpl<PeerFd, epoll_wrapper::Epoll<PeerFd>, MockBufferedQueueManager, EpollMsgQueue>;

void writeToFd(int fd, std::string text)
{
    int s = write(fd, text.c_str(), text.size());
    assert(s > 0);
}

std::pair<int, PeerFd> createPeer()
{
    int32_t pipeFds[2];
    int pipeRes = pipe(pipeFds);

    // Set file descriptor to non blocking
    fcntl(pipeFds[0], F_SETFL, O_NONBLOCK);

    assert(pipeRes == 0);

    http::PeerId pId("host", 0);
    return {pipeFds[1], PeerFd{pId, pipeFds[0]}};
}

TEST(CONNECTION_READ, sub_and_unsub)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll =
        epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    EpollMsgQueue epollQueue;
    auto queue = epollQueue.getLeftEnd();
    MockBufferedQueueManager bqm;
    MockReadHandler mr(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto t = std::thread(
        [&]()
        {
            mr.run();
        });

    auto [writeFd1, peer1] = createPeer();
    auto [writeFd2, peer2] = createPeer();
    queue.push(Subscribe(peer1));
    mr.notify();
    queue.push(Subscribe(peer2));
    mr.notify();
    queue.push(UnSubscribe(peer1));
    mr.notify();
    queue.push(UnSubscribe(peer2));
    mr.notify();

    queue.push(Deactivate{});
    mr.notify();

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

    ASSERT_FALSE(mr.isActive());
}

TEST(CONNECTION_READ, one_subscriber_read_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll =
        epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    EpollMsgQueue epollQueue;
    auto queue = epollQueue.getLeftEnd();
    std::mutex mutex;
    std::condition_variable cv;
    epollQueue.getRightEnd().attachNotifier(mutex, cv);
    epollQueue.getLeftEnd().attachNotifier(mutex, cv);
    MockBufferedQueueManager bqm;
    MockReadHandler mr(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto [writeFd, peer] = createPeer();
    queue.push(Subscribe{peer});
    mr.notify();
    writeToFd(writeFd, "\x04test");
    auto t = std::thread(
        [&]()
        {
            mr.run();
        });

    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock,
                [&]
                {
                    return queue.canPop();
                });
    }

    CtlResponse peerResp{peer, ""};
    ASSERT_EQ(std::get<CtlResponse>(queue.pop()), peerResp);

    {
        std::unique_lock<std::mutex> _l(mutex);
        cv.wait(_l,
                [&]()
                {
                    return queue.canPop();
                });
    }
    ReadEvent re = std::get<ReadEvent>(queue.pop());
    ASSERT_READ(re, testing::ContainerEq<std::vector<char>>({'t', 'e', 's', 't'}));

    queue.push(Deactivate{});
    mr.notify();

    t.join();

    ASSERT_FALSE(mr.isActive());
}

TEST(CONNECTION_READ, one_subscribed_read_multiple)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll =
        epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    EpollMsgQueue epollQueue;
    auto queue = epollQueue.getLeftEnd();
    std::mutex mutex;
    std::condition_variable cv;
    epollQueue.getLeftEnd().attachNotifier(mutex, cv);
    epollQueue.getRightEnd().attachNotifier(mutex, cv);
    MockBufferedQueueManager bqm;
    MockReadHandler mr(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto [writeFd, peer] = createPeer();

    queue.push(Subscribe{peer});
    mr.notify();

    writeToFd(writeFd, "\x04test");

    auto t = std::thread(
        [&]()
        {
            mr.run();
        });

    writeToFd(writeFd, "\x05test1");
    writeToFd(writeFd, "\x05test2");
    writeToFd(writeFd, "\x05test3");

    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock,
                [&]
                {
                    return queue.numToRead() >= 5;
                });
    }

    CtlResponse ctl = std::get<CtlResponse>(queue.pop());
    ReadEvent re1 = std::get<ReadEvent>(queue.pop());
    ReadEvent re2 = std::get<ReadEvent>(queue.pop());
    ReadEvent re3 = std::get<ReadEvent>(queue.pop());
    ReadEvent re4 = std::get<ReadEvent>(queue.pop());
    ASSERT_TRUE(!queue.canPop());

    ASSERT_READ(re1, testing::ContainerEq<std::vector<char>>({'t', 'e', 's', 't'}));

    ASSERT_READ(re2, testing::ContainerEq<std::vector<char>>({'t', 'e', 's', 't', '1'}));

    ASSERT_READ(re3, testing::ContainerEq<std::vector<char>>({'t', 'e', 's', 't', '2'}));

    ASSERT_READ(re4, testing::ContainerEq<std::vector<char>>({'t', 'e', 's', 't', '3'}));

    queue.push(Deactivate{});
    mr.notify();

    t.join();
}

TEST(CONNECTION_READ, multiple_subscriber_read_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll =
        epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    EpollMsgQueue epollQueue;
    auto queue = epollQueue.getLeftEnd();
    std::mutex mutex;
    std::condition_variable cv;
    epollQueue.getRightEnd().attachNotifier(mutex, cv);
    epollQueue.getLeftEnd().attachNotifier(mutex, cv);
    MockBufferedQueueManager bqm;
    MockReadHandler mr(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto [writeFd1, peer1] = createPeer();
    auto [writeFd2, peer2] = createPeer();
    auto [writeFd3, peer3] = createPeer();

    queue.push(Subscribe{peer1});
    queue.push(Subscribe{peer2});
    queue.push(Subscribe{peer3});

    mr.notify();

    writeToFd(writeFd1, "\x05peer1");
    writeToFd(writeFd2, "\x05peer2");
    writeToFd(writeFd3, "\x05peer3");

    auto t = std::thread(
        [&]()
        {
            mr.run();
        });

    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock,
                [&]
                {
                    return queue.numToRead() >= 6;
                });
    }

    CtlResponse resp1{peer1, ""};
    CtlResponse resp2{peer2, ""};
    CtlResponse resp3{peer3, ""};

    ASSERT_EQ(std::get<CtlResponse>(queue.pop()), resp1);
    ASSERT_EQ(std::get<CtlResponse>(queue.pop()), resp2);
    ASSERT_EQ(std::get<CtlResponse>(queue.pop()), resp3);

    ReadEvent re1 = std::get<ReadEvent>(queue.pop());
    ReadEvent re2 = std::get<ReadEvent>(queue.pop());
    ReadEvent re3 = std::get<ReadEvent>(queue.pop());

    ASSERT_READ(re1, testing::ContainerEq<std::vector<char>>({'p', 'e', 'e', 'r', '1'}));
    ASSERT_READ(re2, testing::ContainerEq<std::vector<char>>({'p', 'e', 'e', 'r', '2'}));
    ASSERT_READ(re3, testing::ContainerEq<std::vector<char>>({'p', 'e', 'e', 'r', '3'}));

    queue.push(Deactivate{});
    mr.notify();

    t.join();
}

TEST(CONNECTION_READ, multiple_subscribed_read_multiple)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll =
        epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    EpollMsgQueue epollQueue;
    auto queue = epollQueue.getLeftEnd();
    std::mutex mutex;
    std::condition_variable cv;
    epollQueue.getRightEnd().attachNotifier(mutex, cv);
    epollQueue.getLeftEnd().attachNotifier(mutex, cv);
    MockBufferedQueueManager bqm;
    MockReadHandler mr(epoll.getEpoll(), bqm, epollQueue.getRightEnd());

    auto t = std::thread(
        [&]()
        {
            mr.run();
        });

    constexpr int numPeers = 5;
    std::vector<PeerFd> peers;
    for (int p = 0; p < numPeers; p++)
    {
        auto [writeFd, peerFd] = createPeer();

        queue.push(Subscribe{peerFd});
        mr.notify();
        peers.push_back(peerFd);
        writeToFd(writeFd, "\x0dmsg received" + std::to_string(p));

        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock,
                    [&]
                    {
                        return queue.numToRead() >= (p + 1) * 2;
                    });
        }
    }

    for (int p = 0; p < numPeers; p++)
    {
        const auto resp = CtlResponse{peers[p], ""};
        ASSERT_EQ(resp, std::get<CtlResponse>(queue.pop()));

        ReadEvent re = std::get<ReadEvent>(queue.pop());
        ASSERT_READ(re, testing::ContainerEq<std::vector<char>>({'m', 's', 'g', ' ', 'r', 'e', 'c',
                                                                 'e', 'i', 'v', 'e', 'd',
                                                                 static_cast<char>(p + '0')}));
    }

    queue.push(Deactivate{});
    mr.notify();

    t.join();
}

} // namespace fractals::network::p2p