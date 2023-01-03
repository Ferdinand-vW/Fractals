#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/Peer.h"
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
using namespace fractals::network::http;

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

    assert(pipeRes == 0);

    PeerId pId{"host", 0};
    return {pipeFds[0], PeerFd{pId, Socket(pipeFds[1])}};
}

// TEST(CONNECTION_WRITE, sub_and_unsub)
// {
//     epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

//     ASSERT_TRUE(epoll);

//     WorkQueue rq;
//     ConnectionWriteHandler mr(epoll.getEpoll(), rq);

//     auto t = std::thread([&](){
//         mr.run();
//     });

//     auto [readFd1, peer1] = createPeer();
//     auto ctl1 = mr.subscribe(peer1);
//     ASSERT_FALSE(ctl1.hasError());

//     auto [readFd2, peer2] = createPeer();
//     auto ctl2 = mr.subscribe(peer2);
//     ASSERT_FALSE(ctl2.hasError());

//     auto ctl3 = mr.unsubscribe(peer1);
//     ASSERT_FALSE(ctl3.hasError());

//     auto ctl4 = mr.unsubscribe(peer2);
//     ASSERT_FALSE(ctl4.hasError());

//     mr.stop();

//     t.join();

// }

// TEST(CONNECTION_WRITE, one_subscriber_write_one)
// {
//     epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

//     ASSERT_TRUE(epoll);

//     WorkQueue rq;
//     ConnectionWriteHandler mr(epoll.getEpoll(), rq);

//     auto [readFd, peer] = createPeer();

//     auto ctl = mr.subscribe(peer);

//     ASSERT_FALSE(ctl.hasError());

//     writeToFd(readFd, "test");

//     auto t = std::thread([&](){
//         mr.run();
//     });

//     // Give epoll thread enough time to wait
//     this_thread::sleep_for(std::chrono::milliseconds(10));

//     ASSERT_TRUE(!rq.isEmpty());
//     PeerEvent pe = rq.pop();

//     ASSERT_TRUE(rq.isEmpty());

//     ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe));

//     auto re = std::get<ReceiveEvent>(pe);
//     ASSERT_EQ(re.mData, std::deque<char>({'t','e','s','t'}));

//     mr.stop();

//     t.join();
// }

// TEST(CONNECTION_WRITE, one_subscribed_write_multiple)
// {
//     epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

//     ASSERT_TRUE(epoll);

//     WorkQueue rq;
//     ConnectionWriteHandler mr(epoll.getEpoll(), rq);

//     auto [readFd, peer] = createPeer();

//     auto ctl = mr.subscribe(peer);

//     ASSERT_FALSE(ctl.hasError());

//     writeToFd(readFd, "test");

//     auto t = std::thread([&](){
//         mr.run();
//     });

//     // Give epoll thread enough time to wait
//     this_thread::sleep_for(std::chrono::milliseconds(10));

//     writeToFd(readFd, "test1");

//     this_thread::sleep_for(std::chrono::milliseconds(10));
//     writeToFd(readFd, "test2");

//     this_thread::sleep_for(std::chrono::milliseconds(10));
//     writeToFd(readFd, "test3");

//     this_thread::sleep_for(std::chrono::milliseconds(10));

//     ASSERT_TRUE(!rq.isEmpty());
//     ASSERT_EQ(rq.size(), 4);
//     PeerEvent pe = rq.pop();
//     PeerEvent p2 = rq.pop();
//     PeerEvent p3 = rq.pop();
//     PeerEvent p4 = rq.pop();
//     ASSERT_TRUE(rq.isEmpty());

//     ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe));
//     auto re = std::get<ReceiveEvent>(pe);
//     ASSERT_EQ(re.mData, std::deque<char>({'t','e','s','t'}));

//     ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(p2));
//     auto re2 = std::get<ReceiveEvent>(p2);
//     ASSERT_EQ(re2.mData, std::deque<char>({'t','e','s','t','1'}));

//     ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(p3));
//     auto re3 = std::get<ReceiveEvent>(p3);
//     ASSERT_EQ(re3.mData, std::deque<char>({'t','e','s','t','2'}));

//     ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(p4));
//     auto re4 = std::get<ReceiveEvent>(p4);
//     ASSERT_EQ(re4.mData, std::deque<char>({'t','e','s','t','3'}));

//     mr.stop();

//     t.join();
// }

// TEST(CONNECTION_WRITE, multiple_subscriber_write_one)
// {
//     epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

//     ASSERT_TRUE(epoll);

//     WorkQueue rq;
//     ConnectionWriteHandler mr(epoll.getEpoll(), rq);

//     auto [readFd1, peer1] = createPeer();
//     auto [readFd2, peer2] = createPeer();
//     auto [readFd3, peer3] = createPeer();

//     auto ctl1 = mr.subscribe(peer1);
//     ASSERT_FALSE(ctl1.hasError());
//     auto ctl2 = mr.subscribe(peer2);
//     ASSERT_FALSE(ctl2.hasError());
//     auto ctl3 = mr.subscribe(peer3);
//     ASSERT_FALSE(ctl3.hasError());

//     writeToFd(readFd1, "peer1");
//     writeToFd(readFd2, "peer2");
//     writeToFd(readFd3, "peer3");

//     auto t = std::thread([&](){
//         mr.run();
//     });

//     // Give epoll thread enough time to wait
//     this_thread::sleep_for(std::chrono::milliseconds(10));

//     ASSERT_TRUE(!rq.isEmpty());
//     ASSERT_EQ(rq.size(), 3);
//     PeerEvent pe1 = rq.pop();
//     PeerEvent pe2 = rq.pop();
//     PeerEvent pe3 = rq.pop();
//     ASSERT_TRUE(rq.isEmpty());

//     ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe1));
//     ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe2));
//     ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe3));

//     auto re1 = std::get<ReceiveEvent>(pe1);
//     auto re2 = std::get<ReceiveEvent>(pe2);
//     auto re3 = std::get<ReceiveEvent>(pe3);
//     ASSERT_EQ(re1.mData, std::deque<char>({'p','e','e','r','1'}));
//     ASSERT_EQ(re2.mData, std::deque<char>({'p','e','e','r','2'}));
//     ASSERT_EQ(re3.mData, std::deque<char>({'p','e','e','r','3'}));

//     mr.stop();

//     t.join();
// }

// TEST(CONNECTION_WRITE, multiple_subscribed_write_multiple)
// {
//     epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

//     ASSERT_TRUE(epoll);

//     WorkQueue rq;
//     ConnectionWriteHandler mr(epoll.getEpoll(), rq);

//     auto t = std::thread([&](){
//         mr.run();
//     });

//     constexpr int numPeers = 5;
//     constexpr int numMsg = 4;
//     for (int p = 0; p < numPeers; p++)
//     {
//         auto [readFd, peerFd] = createPeer();

//         auto subCtl = mr.subscribe(peerFd);

//         ASSERT_FALSE(subCtl.hasError());

//         for(int m = 0; m < numMsg; m++)
//         {
//             ASSERT_TRUE(mr.isSubscribed(peerFd));
//             writeToFd(readFd, "msg"+to_string(m));
//         }
//     }

//     this_thread::sleep_for(std::chrono::milliseconds(40));

//     ASSERT_EQ(rq.size(), numPeers);

//     for (int p = 0; p << numPeers; p++)
//     {
//         PeerEvent pe = rq.pop();

//         ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(pe));

//         auto re = std::get<ReceiveEvent>(pe);
//         ASSERT_EQ(fractals::common::ascii_decode(re.mData), "msg"+to_string(p));
//     }

//     mr.stop();

//     t.join();
// }