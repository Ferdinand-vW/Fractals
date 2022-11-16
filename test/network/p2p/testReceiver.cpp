#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/p2p/Receiver.ipp"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/torrent/Bencode.h"
#include "neither/maybe.hpp"
#include <epoll_wrapper/EpollImpl.ipp>
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


class FakeEpoll
{
    public:
        FakeEpoll() = default;
        ~FakeEpoll() = default;
        
        static std::unique_ptr<FakeEpoll> epoll_create(int)
        {
            return std::unique_ptr<FakeEpoll>(new FakeEpoll());
        }

        int close()
        {
            return 0;
        }

        int epoll_wait(struct epoll_event* events, int maxevents, int timeout)
        {
            return 0;
        }

        int epoll_ctl(int action, int fd, struct epoll_event* event)
        {
            return 0;
        }      
};

using MockEpoll = epoll_wrapper::EpollImpl<FakeEpoll, PeerFd>;


using MockReceiver = ReceiverWorkerImpl<PeerFd, MockEpoll, WorkQueue>;

TEST(RECEIVER, receiver_mock)
{
    epoll_wrapper::CreateAction<FakeEpoll, PeerFd> epoll = MockEpoll::epollCreate();
    WorkQueue rq;
    MockReceiver mr(epoll.getEpoll(), rq);

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

