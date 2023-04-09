#include "fractals/common/WorkQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/PeerEventQueue.h"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/PeerService.h"

#include "gmock/gmock.h"
#include <epoll_wrapper/Error.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

namespace fractals::network::p2p
{

class MockEvent
{
};

using MockEventQueue = common::WorkQueueImpl<32, MockEvent>;

template <typename Queue> class MockBufferedQueueManager
{
  public:
    MockBufferedQueueManager(MockEventQueue &)
    {
    }

    MOCK_METHOD(void, sendToPeer, (const PeerFd&, const BitTorrentMessage&));
};

template <ActionType AT> class MockEpollService
{
  public:
    using Epoll = int;

    MockEpollService(Epoll &epoll, MockBufferedQueueManager<MockEventQueue> &buffMan){

    };
};

class MockTcpService
{
    public:
    MockTcpService() = default;
    MOCK_METHOD(int32_t, connect, (const std::string&, uint32_t));
};

class PeerServiceTest : public ::testing::Test
{
  public:
    PeerServiceTest() : buffMan(queue), peerService(epoll, epoll, buffMan, tcpService), choke({})
    {
    }

    MockEventQueue queue;
    MockBufferedQueueManager<MockEventQueue> buffMan;
    MockTcpService tcpService;
    int epoll = 0;
    http::PeerId peerId{"", 0};
    PeerServiceImpl<MockEpollService, MockBufferedQueueManager<MockEventQueue>, MockTcpService> peerService;
    Choke choke;
};

TEST_F(PeerServiceTest, onWrite)
{
    EXPECT_CALL(tcpService, connect(_, _)).Times(1).WillOnce(Return(1));
    EXPECT_CALL(buffMan, sendToPeer(_,_)).Times(1);
    peerService.write(peerId, choke);

    EXPECT_CALL(buffMan, sendToPeer(_,_)).Times(1);
    peerService.write(peerId, choke);
}

TEST_F(PeerServiceTest, onWriteToInvalidPeer)
{
    EXPECT_CALL(tcpService, connect(_, _)).Times(1).WillOnce(Return(-1));
    EXPECT_CALL(buffMan, sendToPeer(_,_)).Times(0);
    peerService.write(peerId, choke);

    EXPECT_CALL(tcpService, connect(_, _)).Times(1).WillOnce(Return(-1));
    EXPECT_CALL(buffMan, sendToPeer(_,_)).Times(0);
    peerService.write(peerId, choke);
}

} // namespace fractals::network::p2p