#include "fractals/common/WorkQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/PeerEvent.h"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/PeerService.h"

#include <epoll_wrapper/EpollImpl.h>
#include <epoll_wrapper/Error.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <variant>

using namespace ::testing;
using ::testing::Return;
using namespace std::chrono_literals;

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
    MOCK_METHOD(bool, isActive, ());
};

class PeerServiceTest : public ::testing::Test
{
  public:
    static constexpr http::PeerId PEERID{"1.1.1.1", 1000};
    static constexpr PeerFd PEERFD{PEERID, 1};

    PeerServiceTest()
        : epollService(epoll, queue.getRightEnd()),
          peerService(queue.getLeftEnd(), epollService, tcpService), choke({})
    {
    }

    void doConnect()
    {
        EXPECT_CALL(tcpService, connect(PEERID.ip, PEERID.port)).WillOnce(Return(1));
        EXPECT_CALL(epollService, notify());
        peerService.connect(PEERID, 0ns);
        ASSERT_EQ(requestQueue.numToRead(), 1);
        const auto msg = requestQueue.pop();
        ASSERT_TRUE(std::holds_alternative<Subscribe>(msg));
    }

    MockTcpService tcpService{};
    int epoll = 0;
    EpollMsgQueue queue;
    EpollMsgQueue::RightEndPoint requestQueue = queue.getRightEnd();
    MockEpollService epollService;
    http::PeerId peerId{"", 0};
    PeerFd peerFd{peerId, 0};
    PeerServiceImpl<MockEpollService, MockTcpService> peerService;
    BitTorrentEncoder encoder;
    Choke choke;
};

TEST_F(PeerServiceTest, onWrite)
{
    doConnect();

    EXPECT_CALL(epollService, notify());
    ASSERT_TRUE(peerService.write(PEERID, choke, 0ns));

    ASSERT_TRUE(requestQueue.numToRead());
    const auto msg = requestQueue.pop();
    ASSERT_TRUE(std::holds_alternative<WriteEvent>(msg));
}

TEST_F(PeerServiceTest, onWriteToInvalidPeer)
{
    doConnect();

    EXPECT_CALL(epollService, notify()).Times(0);
    ASSERT_FALSE(peerService.write(http::PeerId{}, choke, 0ns));
}

TEST_F(PeerServiceTest, onReadEpollError)
{
    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(EpollError{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_EQ(std::get<Shutdown>(peerService.read(0ns).value()), Shutdown{});
    ASSERT_EQ(std::get<Deactivate>(queue.getRightEnd().pop()), Deactivate{});
}

TEST_F(PeerServiceTest, onReadEventResponse)
{
    doConnect();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(ReadEventResponse{PEERFD, {}});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_TRUE(std::holds_alternative<ConnectionDisconnected>(peerService.read(0ns).value()));
    ASSERT_TRUE(std::holds_alternative<UnSubscribe>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onReadWithHandShake)
{
    {
        ASSERT_FALSE(peerService.canRead());
        HandShake hs;
        queue.getRightEnd().push(ReadEvent{PEERFD, encoder.encode(hs)});
        ASSERT_TRUE(peerService.canRead());

        EXPECT_CALL(epollService, notify()).Times(0);
        const auto readEvent = peerService.read(0ns).value();
        ASSERT_TRUE(std::holds_alternative<Message>(readEvent));
        const auto msg = std::get<Message>(readEvent);
        ASSERT_TRUE(std::holds_alternative<HandShake>(msg.message));
        ASSERT_EQ(msg.peer, PEERID);
        ASSERT_FALSE(peerService.canRead());
    }

    {
        Bitfield bf;
        queue.getRightEnd().push(ReadEvent{PEERFD, encoder.encode(bf)});
        ASSERT_TRUE(peerService.canRead());

        EXPECT_CALL(epollService, notify()).Times(0);
        const auto readEvent = peerService.read(0ns).value();
        ASSERT_TRUE(std::holds_alternative<Message>(readEvent));
        const auto msg = std::get<Message>(readEvent);
        ASSERT_TRUE(std::holds_alternative<Bitfield>(msg.message));
        ASSERT_EQ(msg.peer, PEERID);
        ASSERT_FALSE(peerService.canRead());
    }
}

TEST_F(PeerServiceTest, onReadWithNoHandShake)
{
    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(ReadEvent{PEERFD, {}});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(0);
    const auto msg = peerService.read(0ns).value();
    ASSERT_TRUE(std::holds_alternative<ConnectionDisconnected>(msg));
}

TEST_F(PeerServiceTest, onWriteEventResponse)
{
    doConnect();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(WriteEventResponse{PEERFD, "error"});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_TRUE(std::holds_alternative<ConnectionDisconnected>(peerService.read(0ns).value()));
    ASSERT_TRUE(std::holds_alternative<UnSubscribe>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onEmptyCtlResponse)
{
    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(CtlResponse{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(0);
    ASSERT_FALSE(peerService.read(0ns));
}

TEST_F(PeerServiceTest, onErrorCtlResponse)
{
    doConnect();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(CtlResponse{PEERFD, "error"});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(1);
    ASSERT_TRUE(std::holds_alternative<ConnectionDisconnected>(peerService.read(0ns).value()));
    ASSERT_TRUE(std::holds_alternative<UnSubscribe>(queue.getRightEnd().pop()));
}

TEST_F(PeerServiceTest, onConnectionCloseEvent)
{
    doConnect();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(ConnectionCloseEvent{PEERID});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(0);
    ASSERT_TRUE(std::holds_alternative<ConnectionDisconnected>(peerService.read(0ns).value()));
    ASSERT_FALSE(requestQueue.numToRead());
}

TEST_F(PeerServiceTest, onConnectionError)
{
   doConnect();

    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(ConnectionError{PEERID});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(0);
    ASSERT_TRUE(std::holds_alternative<ConnectionDisconnected>(peerService.read(0ns).value()));
    ASSERT_FALSE(requestQueue.numToRead());
}

TEST_F(PeerServiceTest, onShutdown)
{
    ASSERT_FALSE(peerService.canRead());
    queue.getRightEnd().push(EpollError{});
    ASSERT_TRUE(peerService.canRead());

    EXPECT_CALL(epollService, notify()).Times(0);
}

} // namespace fractals::network::p2p