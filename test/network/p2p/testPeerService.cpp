#include "fractals/common/WorkQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/PeerService.h"

#include "gmock/gmock.h"
#include <epoll_wrapper/EpollImpl.h>
#include <epoll_wrapper/Error.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <variant>

using ::testing::_;
using ::testing::Return;

namespace fractals::network::p2p
{

class MockEpollService
{
  public:
    using Epoll = int;

    MockEpollService(Epoll &epoll, EpollMsgQueue::RightEndPoint queue) : queue(queue){};

    MOCK_METHOD(void, notify, ());

    bool isActive()
    {
        return true;
    }

    EpollMsgQueue::RightEndPoint queue;
};

class MockTcpService
{
  public:
    MockTcpService() = default;
    MOCK_METHOD(int32_t, connect, (const std::string &, uint32_t));
};

class PeerServiceTest : public ::testing::Test
{
  public:
    PeerServiceTest()
        : epollService(epoll, queue.getRightEnd()), peerService(queue.getLeftEnd(), epollService, tcpService), choke({})
    {
    }

    void addPeerToDict()
    {
        EXPECT_CALL(tcpService, connect(_,_)).WillOnce(Return(0));
        peerService.write(peerId, KeepAlive{});
        queue.getRightEnd().pop();
        queue.getRightEnd().pop();
    }

    MockTcpService tcpService{};
    int epoll = 0;
    EpollMsgQueue queue;
    MockEpollService epollService;
    http::PeerId peerId{"", 0};
    PeerFd peerFd{peerId, 0};
    PeerServiceImpl<MockEpollService, MockTcpService> peerService;
    Choke choke;
};

TEST_F(PeerServiceTest, onWrite)
{
    EXPECT_CALL(tcpService, connect(_, _)).Times(1).WillOnce(Return(1));
    EXPECT_CALL(epollService, notify()).Times(1);
    peerService.write(peerId, choke);

    ASSERT_TRUE(std::holds_alternative<Subscribe>(queue.getRightEnd().pop()));
    ASSERT_TRUE(std::holds_alternative<WriteEvent>(queue.getRightEnd().pop()));

    EXPECT_CALL(tcpService, connect(_, _)).Times(0);
    EXPECT_CALL(epollService, notify()).Times(1);
    peerService.write(peerId, choke);

    ASSERT_TRUE(std::holds_alternative<WriteEvent>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onWriteToInvalidPeer)
{
    EXPECT_CALL(tcpService, connect(_, _)).Times(1).WillOnce(Return(-1));
    EXPECT_CALL(epollService, notify()).Times(0);
    peerService.write(peerId, choke);

    EXPECT_CALL(tcpService, connect(_, _)).Times(1).WillOnce(Return(-1));
    EXPECT_CALL(epollService, notify()).Times(0);
    peerService.write(peerId, choke);
}

TEST_F(PeerServiceTest, onReadEpollError)
{
    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(EpollError{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_EQ(std::get<Shutdown>(peerService.read().value()), Shutdown{});
    ASSERT_EQ(std::get<Deactivate>(queue.getRightEnd().pop()), Deactivate{});
}

TEST_F(PeerServiceTest, onReadEventResponse)
{
    addPeerToDict();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(ReadEventResponse{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_TRUE(std::holds_alternative<Disconnect>(peerService.read().value()));
    ASSERT_TRUE(std::holds_alternative<UnSubscribe>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onReadEvent)
{
    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(ReadEvent{PeerFd{peerId, 0}, {}});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(0);
    Message m{peerId, KeepAlive{}};
    ASSERT_EQ(std::get<Message>(peerService.read().value()), m);
}

TEST_F(PeerServiceTest, onWriteEventResponse)
{
    addPeerToDict();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(WriteEventResponse{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_TRUE(std::holds_alternative<Disconnect>(peerService.read().value()));
    ASSERT_TRUE(std::holds_alternative<UnSubscribe>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onEmptyCtlResponse)
{
    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(CtlResponse{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(0);
    ASSERT_FALSE(peerService.read());
}

TEST_F(PeerServiceTest, onErrorCtlResponse)
{
    addPeerToDict();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(CtlResponse{peerFd, "error"});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_TRUE(std::holds_alternative<Disconnect>(peerService.read().value()));
    ASSERT_TRUE(std::holds_alternative<UnSubscribe>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onConnectionCloseEvent)
{
    addPeerToDict();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(ConnectionCloseEvent{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_TRUE(std::holds_alternative<Disconnect>(peerService.read().value()));
    ASSERT_TRUE(std::holds_alternative<UnSubscribe>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onConnectionError)
{
    addPeerToDict();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(ConnectionError{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_TRUE(std::holds_alternative<Disconnect>(peerService.read().value()));
    ASSERT_TRUE(std::holds_alternative<UnSubscribe>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onDeactivationResponse)
{
    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(DeactivateResponse{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(0);
}

} // namespace fractals::network::p2p