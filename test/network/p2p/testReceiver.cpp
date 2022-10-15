#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/p2p/Receiver.h"
#include "fractals/torrent/Bencode.h"
#include "neither/maybe.hpp"
#include <epoll_wrapper/EpollImpl.h>
#include <fractals/network/http/Tracker.h>
#include <fractals/torrent/MetaInfo.h>

#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>

using ::testing::StrictMock;
using namespace fractals::network::p2p;

// class FakeEpoll
// {
//     std::unordered_map<Socket, std::optional<std::vector<char>>> listeners;
//     int efd{-1};

//     public:
//         FakeEpoll()
//         {
//             efd = epoll_create(1024);
//         }

//         ~FakeEpoll()
//         {
//             close(efd);
//         }

//         FakeEpoll(const FakeEpoll&) = delete;
//         FakeEpoll(FakeEpoll&&) = delete;
//         FakeEpoll& operator=(const FakeEpoll&) = delete;
//         FakeEpoll& operator=(FakeEpoll&&) = delete;

//         int wait(struct epoll_event *events)
//         {
//             return 0;
//         }

//         int addListener(Socket fd)
//         {
//             return 0;
//         }

//         void setData(Socket fd, std::vector<char> data)
//         {
//             listeners[fd] = data;
//         }

//         void removeListener(Socket fd)
//         {
//             listeners.erase(fd);
//         }
        
// };

// class MockEpoll
// {

//     public:
//         MockEpoll()
//         {
//             ON_CALL(*this, wait).WillByDefault([this](struct epoll_event *events) {
//                 return fake_.wait(events);
//             });
//             ON_CALL(*this, addListener).WillByDefault([this](Socket fd) {
//                 return fake_.addListener(fd);
//             });

//             ON_CALL(*this, removeListener).WillByDefault([this](Socket fd) {
//                 fake_.removeListener(fd);
//             });
//         }

//         MOCK_METHOD(int, wait, (struct epoll_event *events));
//         MOCK_METHOD(int, addListener, (Socket));
//         MOCK_METHOD(void, removeListener, (Socket));

//         FakeEpoll fake_;
// };

using MockReceiver = ReceiverWorkerImpl<PeerFd, epoll_wrapper::Epoll, ReceiveQueue<PeerEvent>>;

TEST(RECEIVER, receiver_mock)
{
    auto epoll = epoll_wrapper::Epoll<PeerFd>::epollCreate();
    ReceiveQueue<Peer> rq;
    MockReceiver mr(epoll.mEpoll.get(), rq);

    // ReceiveQueue rq{};
    // ActivePeers ap{};
    // StrictMock<MockEpoll> me;
    // ReceiverWorker rw(me, ap, rq);

    // auto peerSocket = ap.newPeer(PeerAddress("1",0));
    // auto peerSocket2 = ap.newPeer(PeerAddress("2",0));

    // EXPECT_CALL(me, addListener).Times(2);
    // rw.addPeer(peerSocket.value);
    // rw.addPeer(peerSocket2.value);

    // EXPECT_CALL(me, removeListener).Times(1);
    // rw.removePeer(peerSocket.value);

    // EXPECT_CALL(me, wait).Times(1);
    // rw.runOnce(); // how long does it run?
}

TEST(RECEIVER, populate_receive_queue_on_messages)
{
    // ReceiveQueue<MockMessage> rq{};
    // ActivePeers ap{};
    // StrictMock<MockEpoll> me;
    // ReceiverWorker rw(me, ap, rq);

    // auto peerSocket = ap.newPeer(PeerAddress("1",0));

    // MockMessage m("a");
    // me.setData(peerSocket.value, m);

    // rw.runOnce();

    // ASSERT_EQ(rq.front().value, "a");
}

