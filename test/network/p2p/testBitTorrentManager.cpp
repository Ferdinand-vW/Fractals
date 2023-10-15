#include "fractals/app/Event.h"
#include "fractals/common/Tagged.h"
#include "fractals/common/TcpService.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentManager.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollService.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/PeerEvent.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/PersistEventQueue.h"

#include <epoll_wrapper/Error.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ostream>

using ::testing::_;
using ::testing::Return;

namespace fractals::network::p2p
{

// std::ostream& operator<<(std::ostream& os, const PeerEvent& event)
// {
//     std::visit(common::overloaded{
//         [&](const auto&){},
//     }, event);

//     return os;
// }

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
    BitTorrentManagerMock(MockPeerService &peerService,
                          persist::PersistEventQueue::LeftEndPoint persistQueue,
                          disk::DiskEventQueue &diskQueue)
        : peerService(peerService), eventHandler{this}
    {
    }

    void eval()
    {
        eventHandler.handleEvent(peerService.read().value());
    }

    MOCK_METHOD(void, shutdown, ());
    MOCK_METHOD(void, disconnectPeer, (http::PeerId));
    MOCK_METHOD(void, peerCompletedTorrent, (http::PeerId, common::InfoHash));
    MOCK_METHOD(void, process, (const app::AddedTorrent &));
    MOCK_METHOD(void, process, (const app::RemoveTorrent &));
    MOCK_METHOD(void, process, (const app::StopTorrent &));
    MOCK_METHOD(void, process, (const app::StartTorrent &));
    MOCK_METHOD(void, process, (const app::ResumeTorrent &));
    MOCK_METHOD(void, process, (const app::Shutdown &));
    MOCK_METHOD(void, process, (const app::RequestStats &));
    MOCK_METHOD(void, process, (const persist::AddedTorrent &));
    MOCK_METHOD(void, process, (const persist::TorrentExists &));
    MOCK_METHOD(void, process, (const persist::Pieces &));
    MOCK_METHOD(void, process, (const persist::Trackers &));
    MOCK_METHOD(void, process, (const persist::Announces &));
    MOCK_METHOD(void, process, (const disk::ReadSuccess &));
    MOCK_METHOD(void, process, (const disk::ReadError &));
    MOCK_METHOD(void, process, (const disk::WriteSuccess &));
    MOCK_METHOD(void, process, (const disk::WriteError &));
    MOCK_METHOD(void, process, (const disk::TorrentInitialized &));
    MOCK_METHOD(void, process, (const http::Announce &));
    MOCK_METHOD(void, process, (const p2p::ConnectionDisconnected &));
    MOCK_METHOD(void, process, (const p2p::ConnectionEstablished &));

    template <typename T>
    std::pair<ProtocolState, common::InfoHash> forwardToPeer(http::PeerId peer, const T &t)
    {
        return forwardToPeerMock(peer, std::move(t));
    }

    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const Choke &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const UnChoke &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const Interested &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const NotInterested &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const Have &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const Bitfield &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const Request &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const Piece &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const Cancel &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const Port &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const KeepAlive &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const HandShake &));
    MOCK_METHOD((std::pair<ProtocolState, common::InfoHash>), forwardToPeerMock,
                (http::PeerId, const SerializeError &));

    MockPeerService &peerService;
    PeerEventHandler<BitTorrentManagerMock> eventHandler;
};

class BitTorrentManagerTest : public ::testing::Test
{
  public:
    static constexpr common::InfoHash INFOHASH{"abcdef"};

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
    EXPECT_CALL(btManMock, disconnectPeer(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeerMock(dummyPeer, Choke{}))
        .Times(1)
        .WillOnce(Return(std::make_pair(ProtocolState::OPEN, INFOHASH)));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolHashCheckFailure)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{dummyPeer, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, disconnectPeer(_)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeerMock(dummyPeer, std::move(Choke{})))
        .Times(1)
        .WillOnce(Return(std::make_pair(ProtocolState::HASH_CHECK_FAIL, INFOHASH)));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolClose)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{dummyPeer, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, disconnectPeer(dummyPeer)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeerMock(dummyPeer, std::move(Choke{})))
        .Times(1)
        .WillOnce(Return(std::make_pair(ProtocolState::CLOSED, INFOHASH)));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolError)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{dummyPeer, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(1);
    EXPECT_CALL(btManMock, disconnectPeer(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeerMock(dummyPeer, std::move(Choke{})))
        .Times(1)
        .WillOnce(Return(std::make_pair(ProtocolState::ERROR, INFOHASH)));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onDisconnectEvent)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(ConnectionDisconnected{dummyPeer}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, process(ConnectionDisconnected{dummyPeer})).Times(1);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onShutdownEvent)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Shutdown{}));

    EXPECT_CALL(btManMock, shutdown()).Times(1);
    EXPECT_CALL(btManMock, disconnectPeer(_)).Times(0);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, dropOnPeerError)
{
    // btManMock.
}

} // namespace fractals::network::p2p