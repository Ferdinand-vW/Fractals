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

TEST(RECEIVER, receiver_mock)
{
    epoll_wrapper::CreateAction<epoll_wrapper::Epoll<PeerFd>> epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();

    ASSERT_TRUE(epoll);

    WorkQueue rq;
    MockReceiver mr(epoll.getEpoll(), rq);

    auto [writeFd, peer] = createPeer();

    std::cout << peer.getFileDescriptor() << std::endl;

    auto ctl = mr.subscribe(peer);

    ASSERT_FALSE(ctl.hasError());

    auto t = std::thread([&](){
        mr.run();
    });

    writeToFd(writeFd, "test");

    // Make sure we have written and read using epoll before checking queue
    this_thread::sleep_for(std::chrono::milliseconds(100));

    // std::cout << v.size() << std::endl;

    ASSERT_TRUE(!rq.isEmpty());
    PeerEvent pe = rq.pop();

    mr.stop();

    t.join();
}

TEST(RECEIVER, populate_receive_queue_on_messages)
{
   
}

