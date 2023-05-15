#include "fractals/common/TcpService.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentManager.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollService.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/persist/PersistEventQueue.h"

#include "gmock/gmock.h"
#include <epoll_wrapper/Error.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

namespace fractals::network::p2p
{

const auto dummyPeer = http::PeerId{"host", 0};

class MockPeerService
{
  public:
    MOCK_METHOD(std::optional<PeerEvent>, read, ());
};

class BitTorrentManagerMock
{
    using Base = BitTorrentManager;

  public:
    BitTorrentManagerMock(MockPeerService &peerService, persist::PersistEventQueue::LeftEndPoint persistQueue,
                          disk::DiskEventQueue &diskQueue)
        : peerService(peerService), eventHandler{this}
    {
    }

    void eval()
    {
        eventHandler.handleEvent(peerService.read().value());
    }

    MOCK_METHOD(void, shutdown, ());
    MOCK_METHOD(void, dropPeer, (http::PeerId));
    template <typename T> ProtocolState forwardToPeer(http::PeerId peer, T &&t)
    {
        return forwardToPeer(peer, std::move(t));
    }

    MOCK_METHOD(ProtocolState, forwardToPeer, (http::PeerId, Choke &&));

    MockPeerService &peerService;
    BitTorrentEventHandler<BitTorrentManagerMock> eventHandler;
};

class BitTorrentManagerTest : public ::testing::Test
{
  public:
    BitTorrentManagerTest()
    {
    }

    MockPeerService peerService{};
    persist::PersistEventQueue persistQueue;
    disk::DiskEventQueue diskQueue;

    BitTorrentManagerMock btManMock{peerService, persistQueue.getLeftEnd(), diskQueue};
};

TEST_F(BitTorrentManagerTest, onValidPeerEvent)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{dummyPeer, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, dropPeer(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(dummyPeer, Choke{})).Times(1).WillOnce(Return(ProtocolState::OPEN));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolHashCheckFailure)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{dummyPeer, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, dropPeer(_)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeer(dummyPeer, std::move(Choke{})))
        .Times(1)
        .WillOnce(Return(ProtocolState::HASH_CHECK_FAIL));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolClose)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{dummyPeer, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, dropPeer(dummyPeer)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeer(dummyPeer, std::move(Choke{}))).Times(1).WillOnce(Return(ProtocolState::CLOSED));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolError)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{dummyPeer, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(1);
    EXPECT_CALL(btManMock, dropPeer(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(dummyPeer, std::move(Choke{}))).Times(1).WillOnce(Return(ProtocolState::ERROR));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onDisconnectEvent)
{
    EXPECT_CALL(peerService, read())
        .Times(1)
        .WillOnce(Return(Disconnect{dummyPeer}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, dropPeer(dummyPeer)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeer(_, std::move(_))).Times(0);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onShutdownEvent)
{
    EXPECT_CALL(peerService, read())
        .Times(1)
        .WillOnce(Return(Shutdown{}));

    EXPECT_CALL(btManMock, shutdown()).Times(1);
    EXPECT_CALL(btManMock, dropPeer(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(_, _)).Times(0);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, dropOnPeerError)
{
    // btManMock.
}

} // namespace fractals::network::p2p