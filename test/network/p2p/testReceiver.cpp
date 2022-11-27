#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/Receiver.ipp"
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

using MockReceiver = ReceiverWorkerImpl<PeerFd, epoll_wrapper::Epoll<PeerFd>, WorkQueue>;

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

TEST(RECEIVER, one_subscriber_read_one)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    WorkQueue rq;
    MockReceiver mr(epoll.getEpoll(), rq);

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

TEST(RECEIVER, one_subscribed_read_multiple)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    WorkQueue rq;
    MockReceiver mr(epoll.getEpoll(), rq);

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
    PeerEvent pe = rq.pop();
    PeerEvent p2 = rq.pop();

    ASSERT_TRUE(rq.isEmpty());

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe));

    auto re = std::get<ReceiveEvent>(pe);
    ASSERT_EQ(re.mData, std::deque<char>({'t','e','s','t'}));

    mr.stop();

    t.join();
}

