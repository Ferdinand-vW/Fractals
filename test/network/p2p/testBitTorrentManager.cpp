#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentManager.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/PeerEventQueue.h"
#include "fractals/persist/PersistEventQueue.h"

#include "gmock/gmock.h"
#include <epoll_wrapper/Error.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

namespace fractals::network::p2p
{

bool operator==(const ReceiveEvent &lhs, const ReceiveEvent &rhs)
{
    return lhs.peerId == rhs.peerId && lhs.mMessage == rhs.mMessage;
}

const auto dummyPeer = http::PeerId{"host", 0};

class BitTorrentManagerMock
{
    using Base = BitTorrentManager;

  public:
    BitTorrentManagerMock(PeerEventQueue &peerQueue, BufferedQueueManager &buffMan,
                          persist::PersistEventQueue::LeftEndPoint persistQueue, disk::DiskEventQueue &diskQueue)
        : peerQueue(peerQueue), eventHandler{this}
    {
    }

    void eval()
    {
        eventHandler.handleEvent(peerQueue.pop());
    }

    MOCK_METHOD(void, shutdown, (const std::string &));
    MOCK_METHOD(void, dropPeer, (http::PeerId, const std::string &));
    MOCK_METHOD(void, closeConnection, (http::PeerId));
    MOCK_METHOD(void, onShutdown, (uint8_t));
    MOCK_METHOD(void, onDeactivate, (uint8_t));
    template <typename T> ProtocolState forwardToPeer(http::PeerId peer, T &&t)
    {
        return forwardToPeer(peer, std::move(t));
    }

    MOCK_METHOD(ProtocolState, forwardToPeer, (http::PeerId, Choke &&));

    PeerEventQueue &peerQueue;
    BitTorrentEventHandler<BitTorrentManagerMock> eventHandler;
};

class BitTorrentManagerTest : public ::testing::Test
{
  public:
    BitTorrentManagerTest()
    {
    }

    PeerEventQueue peerQueue;
    BufferedQueueManager buffMan{peerQueue};
    persist::PersistEventQueue persistQueue;
    disk::DiskEventQueue diskQueue;

    BitTorrentManagerMock btManMock{peerQueue, buffMan, persistQueue.getLeftEnd(), diskQueue};
};

TEST_F(BitTorrentManagerTest, onValidPeerEvent)
{
    Choke ck{};
    const auto msg = ReceiveEvent{dummyPeer, ck};
    peerQueue.push(msg);

    EXPECT_CALL(btManMock, shutdown(_)).Times(0);
    EXPECT_CALL(btManMock, dropPeer(_, _)).Times(0);
    EXPECT_CALL(btManMock, closeConnection(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(dummyPeer, std::move(ck))).Times(1).WillOnce(Return(ProtocolState::OPEN));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolHashCheckFailure)
{
    Choke ck{};
    const auto msg = ReceiveEvent{dummyPeer, ck};
    peerQueue.push(msg);

    EXPECT_CALL(btManMock, shutdown(_)).Times(0);
    EXPECT_CALL(btManMock, dropPeer(_, _)).Times(1);
    EXPECT_CALL(btManMock, closeConnection(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(dummyPeer, std::move(ck))).Times(1).WillOnce(Return(ProtocolState::HASH_CHECK_FAIL));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolClose)
{
    Choke ck{};
    const auto msg = ReceiveEvent{dummyPeer, ck};
    peerQueue.push(msg);

    EXPECT_CALL(btManMock, shutdown(_)).Times(0);
    EXPECT_CALL(btManMock, dropPeer(_, _)).Times(0);
    EXPECT_CALL(btManMock, closeConnection(_)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeer(dummyPeer, std::move(ck))).Times(1).WillOnce(Return(ProtocolState::CLOSED));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolError)
{
    Choke ck{};
    const auto msg = ReceiveEvent{dummyPeer, ck};
    peerQueue.push(msg);

    EXPECT_CALL(btManMock, shutdown(_)).Times(1);
    EXPECT_CALL(btManMock, dropPeer(_, _)).Times(0);
    EXPECT_CALL(btManMock, closeConnection(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(dummyPeer, std::move(ck))).Times(1).WillOnce(Return(ProtocolState::ERROR));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onInvalidPeerEvent)
{
    const auto msg = ReceiveError{dummyPeer, epoll_wrapper::ErrorCode::EbadF};
    peerQueue.push(msg);

    EXPECT_CALL(btManMock, shutdown(_)).Times(0);
    EXPECT_CALL(btManMock, dropPeer(dummyPeer, std::to_string(msg.errorCode))).Times(1);
    EXPECT_CALL(btManMock, closeConnection(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(_, std::move(_))).Times(0);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onConnectionCloseEvent)
{
    const auto msg = ConnectionCloseEvent{dummyPeer};
    peerQueue.push(msg);

    EXPECT_CALL(btManMock, shutdown(_)).Times(0);
    EXPECT_CALL(btManMock, dropPeer(_, _)).Times(0);
    EXPECT_CALL(btManMock, closeConnection(dummyPeer)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeer(_, _)).Times(0);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onConnectionErrorEvent)
{
    const auto msg = ConnectionError{dummyPeer, epoll_wrapper::ErrorCode::EbadF};
    peerQueue.push(msg);

    EXPECT_CALL(btManMock, shutdown(_)).Times(0);
    EXPECT_CALL(btManMock, dropPeer(dummyPeer, std::to_string(msg.errorCode))).Times(1);
    EXPECT_CALL(btManMock, closeConnection(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(_, _)).Times(0);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onEpollError)
{
    const auto msg = EpollError{epoll_wrapper::ErrorCode::EbadF};
    peerQueue.push(msg);

    EXPECT_CALL(btManMock, shutdown(std::to_string(msg.errorCode))).Times(1);
    EXPECT_CALL(btManMock, dropPeer(_, _)).Times(0);
    EXPECT_CALL(btManMock, closeConnection(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeer(_, _)).Times(0);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, dropOnPeerError)
{
    // btManMock.
}

} // namespace fractals::network::p2p