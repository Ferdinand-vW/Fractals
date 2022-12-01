#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/ConnectionEventHandler.h"
#include "fractals/network/p2p/ConnectionEventHandler.ipp"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/torrent/Bencode.h"
#include "neither/maybe.hpp"
#include <boost/mp11/algorithm.hpp>
#include <cassert>
#include <chrono>
#include <cstdio>
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

void writeToFd(int fd, std::string text)
{
    write (fd, text.c_str(), text.size());
}

std::vector<char> readFromFd(int fd)
{
    int READSIZE=64*1024;
    std::vector<char> buffer(READSIZE);
    int bytes_read = read(fd, &buffer[0], READSIZE);

    assert(bytes_read > 0);
    return buffer;
}

std::pair<int, PeerFd> createPeer()
{
    int pipeFds[2];
    int pipeRes = pipe(pipeFds);

    // Set file descriptor to non blocking
    fcntl(pipeFds[0], F_SETFL, O_NONBLOCK);

    assert(pipeRes == 0);

    return {pipeFds[1], PeerFd{"host",0, Socket(pipeFds[0])}};
}

TEST(CONNECTION_READ, sub_and_unsub)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    WorkQueue rq;
    ConnectionReadHandler mr(epoll.getEpoll(), rq);

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

    WorkQueue rq;
    ConnectionReadHandler mr(epoll.getEpoll(), rq);

    auto [writeFd, peer] = createPeer();

    auto ctl = mr.subscribe(peer);

    ASSERT_FALSE(ctl.hasError());

    writeToFd(writeFd, "test");

    auto t = std::thread([&](){
        mr.run();
    });

    // Give epoll thread enough time to wait
    this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_TRUE(!rq.isEmpty());
    PeerEvent pe = rq.pop();

    ASSERT_TRUE(rq.isEmpty());

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe));

    auto re = std::get<ReceiveEvent>(pe);
    ASSERT_EQ(re.mData, std::deque<char>({'t','e','s','t'}));

    mr.stop();

    t.join();
}

TEST(CONNECTION_READ, one_subscribed_read_multiple)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    WorkQueue rq;
    ConnectionReadHandler mr(epoll.getEpoll(), rq);

    auto [writeFd, peer] = createPeer();

    auto ctl = mr.subscribe(peer);

    ASSERT_FALSE(ctl.hasError());

    writeToFd(writeFd, "test");

    auto t = std::thread([&](){
        mr.run();
    });

    // Give epoll thread enough time to wait
    this_thread::sleep_for(std::chrono::milliseconds(10));

    writeToFd(writeFd, "test1");

    this_thread::sleep_for(std::chrono::milliseconds(10));
    writeToFd(writeFd, "test2");

    this_thread::sleep_for(std::chrono::milliseconds(10));
    writeToFd(writeFd, "test3");

    this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_TRUE(!rq.isEmpty());
    ASSERT_EQ(rq.size(), 4);
    PeerEvent pe = rq.pop();
    PeerEvent p2 = rq.pop();
    PeerEvent p3 = rq.pop();
    PeerEvent p4 = rq.pop();
    ASSERT_TRUE(rq.isEmpty());

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe));
    auto re = std::get<ReceiveEvent>(pe);
    ASSERT_EQ(re.mData, std::deque<char>({'t','e','s','t'}));

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(p2));
    auto re2 = std::get<ReceiveEvent>(p2);
    ASSERT_EQ(re2.mData, std::deque<char>({'t','e','s','t','1'}));

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(p3));
    auto re3 = std::get<ReceiveEvent>(p3);
    ASSERT_EQ(re3.mData, std::deque<char>({'t','e','s','t','2'}));

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(p4));
    auto re4 = std::get<ReceiveEvent>(p4);
    ASSERT_EQ(re4.mData, std::deque<char>({'t','e','s','t','3'}));

    mr.stop();

    t.join();
}

TEST(CONNECTION_READ, multiple_subscriber_read_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    WorkQueue rq;
    ConnectionReadHandler mr(epoll.getEpoll(), rq);

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
    this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_TRUE(!rq.isEmpty());
    ASSERT_EQ(rq.size(), 3);
    PeerEvent pe1 = rq.pop();
    PeerEvent pe2 = rq.pop();
    PeerEvent pe3 = rq.pop();
    ASSERT_TRUE(rq.isEmpty());

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe1));
    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe2));
    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe3));

    auto re1 = std::get<ReceiveEvent>(pe1);
    auto re2 = std::get<ReceiveEvent>(pe2);
    auto re3 = std::get<ReceiveEvent>(pe3);
    ASSERT_EQ(re1.mData, std::deque<char>({'p','e','e','r','1'}));
    ASSERT_EQ(re2.mData, std::deque<char>({'p','e','e','r','2'}));
    ASSERT_EQ(re3.mData, std::deque<char>({'p','e','e','r','3'}));

    mr.stop();

    t.join();
}

TEST(CONNECTION_READ, multiple_subscribed_read_multiple)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    WorkQueue rq;
    ConnectionReadHandler mr(epoll.getEpoll(), rq);

    auto t = std::thread([&](){
        mr.run();
    });

    constexpr int numPeers = 5;
    constexpr int numMsg = 4;
    for (int p = 0; p < numPeers; p++)
    {
        auto [writeFd, peerFd] = createPeer();

        auto subCtl = mr.subscribe(peerFd);

        ASSERT_FALSE(subCtl.hasError());

        for(int m = 0; m < numMsg; m++)
        {
            ASSERT_TRUE(mr.isSubscribed(peerFd));
            writeToFd(writeFd, "msg"+to_string(m));
        }
    }

    this_thread::sleep_for(std::chrono::milliseconds(40));

    ASSERT_EQ(rq.size(), numPeers);

    for (int p = 0; p << numPeers; p++)
    {
        PeerEvent pe = rq.pop();

        ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe));

        auto re = std::get<ReceiveEvent>(pe);
        ASSERT_EQ(fractals::common::ascii_decode(re.mData), "msg"+to_string(p));
    }

    mr.stop();

    t.join();
}