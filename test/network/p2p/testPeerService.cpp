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

    MockEpollService(Epoll& epoll, EpollMsgQueue::RightEndPoint queue) : queue(queue){
    };

    MOCK_METHOD(void, notify, ());

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
        : epollService(epoll, queue.getRightEnd())
          ,peerService(queue.getLeftEnd(), epollService, tcpService), choke({})
    {
    }

    MockTcpService tcpService{};
    int epoll = 0;
    EpollMsgQueue queue;
    MockEpollService epollService;
    http::PeerId peerId{"", 0};
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

} // namespace fractals::network::p2p