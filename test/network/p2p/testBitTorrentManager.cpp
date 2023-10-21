#include "fractals/app/AppEventQueue.h"
#include "fractals/app/Event.h"
#include "fractals/common/Tagged.h"
#include "fractals/common/TcpService.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/network/http/Event.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentManager.ipp"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollService.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/PeerEvent.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/network/p2p/Protocol.ipp"
#include "fractals/persist/Event.h"
#include "fractals/persist/Models.h"
#include "fractals/persist/PersistEventQueue.h"
#include "fractals/sync/QueueCoordinator.h"

#include <chrono>
#include <epoll_wrapper/Error.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ostream>
#include <variant>

using ::testing::_;
using ::testing::Return;

namespace fractals::network::p2p
{

class MockPeerService
{
  public:
    EpollMsgQueue::LeftEndPoint getQueueEndPoint()
    {
        return queue.getLeftEnd();
    }

    bool connect(http::PeerId, std::chrono::nanoseconds)
    {
        return false;
    }

    void disconnectClient(http::PeerId)
    {
    }

    MOCK_METHOD(std::optional<PeerEvent>, read, ());
    MOCK_METHOD(void, shutdown, ());

  private:
    p2p::EpollMsgQueue queue;
};

class BitTorrentManagerMock
{
    using Base = BitTorrentManager;

  public:
    BitTorrentManagerMock(MockPeerService &peerService)
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
    static constexpr common::InfoHash INFOHASH2{"feghjk"};
    static constexpr http::PeerId PEERID{"host", 1000};

    BitTorrentManagerTest()
    {
    }

    MockPeerService peerService{};
    persist::PersistEventQueue persistQueue;
    persist::PersistEventQueue::RightEndPoint persistResponses = persistQueue.getRightEnd();
    disk::DiskEventQueue diskQueue;
    disk::DiskEventQueue::RightEndPoint diskResponses = diskQueue.getRightEnd();
    http::AnnounceEventQueue annQueue;
    http::AnnounceEventQueue::RightEndPoint annResponses = annQueue.getRightEnd();
    app::AppEventQueue appQueue;
    app::AppEventQueue::RightEndPoint appResponses = appQueue.getRightEnd();

    BitTorrentManagerMock btManMock{peerService};

    sync::QueueCoordinator queueCoordinator;
    BitTorrentManagerImpl<MockPeerService> btManReal{
        queueCoordinator,       peerService,           persistQueue.getLeftEnd(),
        diskQueue.getLeftEnd(), annQueue.getLeftEnd(), appQueue.getLeftEnd()};
};

TEST_F(BitTorrentManagerTest, onValidPeerEvent)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{PEERID, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, disconnectPeer(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeerMock(PEERID, Choke{}))
        .Times(1)
        .WillOnce(Return(std::make_pair(ProtocolState::OPEN, INFOHASH)));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolHashCheckFailure)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{PEERID, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, disconnectPeer(_)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeerMock(PEERID, std::move(Choke{})))
        .Times(1)
        .WillOnce(Return(std::make_pair(ProtocolState::HASH_CHECK_FAIL, INFOHASH)));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolClose)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{PEERID, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, disconnectPeer(PEERID)).Times(1);
    EXPECT_CALL(btManMock, forwardToPeerMock(PEERID, std::move(Choke{})))
        .Times(1)
        .WillOnce(Return(std::make_pair(ProtocolState::CLOSED, INFOHASH)));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onProtocolError)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Message{PEERID, Choke{}}));

    EXPECT_CALL(btManMock, shutdown()).Times(1);
    EXPECT_CALL(btManMock, disconnectPeer(_)).Times(0);
    EXPECT_CALL(btManMock, forwardToPeerMock(PEERID, std::move(Choke{})))
        .Times(1)
        .WillOnce(Return(std::make_pair(ProtocolState::ERROR, INFOHASH)));

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onDisconnectEvent)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(ConnectionDisconnected{PEERID}));

    EXPECT_CALL(btManMock, shutdown()).Times(0);
    EXPECT_CALL(btManMock, process(ConnectionDisconnected{PEERID})).Times(1);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, onShutdownEvent)
{
    EXPECT_CALL(peerService, read()).Times(1).WillOnce(Return(Shutdown{}));

    EXPECT_CALL(btManMock, shutdown()).Times(1);
    EXPECT_CALL(btManMock, disconnectPeer(_)).Times(0);

    btManMock.eval();
}

TEST_F(BitTorrentManagerTest, processAddTorrent)
{
    btManReal.process(app::AddTorrent{"filepath.txt"});

    ASSERT_TRUE(diskResponses.numToRead());
    const auto msg = diskResponses.pop();
    ASSERT_TRUE(std::holds_alternative<disk::Read>(msg));
    const auto rd = std::get<disk::Read>(msg);
    EXPECT_EQ(rd.filepath, "filepath.txt");
}

TEST_F(BitTorrentManagerTest, processRemoveTorrent)
{
    btManReal.process(app::RemoveTorrent{INFOHASH});

    {
        ASSERT_TRUE(persistResponses.numToRead());
        const auto msg = persistResponses.pop();
        ASSERT_TRUE(std::holds_alternative<persist::RemoveTorrent>(msg));
        const auto rt = std::get<persist::RemoveTorrent>(msg);
        EXPECT_EQ(rt.infoHash, INFOHASH);
    }
    {
        ASSERT_TRUE(annResponses.numToRead());
        const auto msg = annResponses.pop();
        ASSERT_TRUE(std::holds_alternative<http::DeleteTrackers>(msg));
        const auto dt = std::get<http::DeleteTrackers>(msg);
        EXPECT_EQ(dt.infoHash, INFOHASH);
    }
    {
        ASSERT_TRUE(appResponses.numToRead());
        const auto msg = appResponses.pop();
        ASSERT_TRUE(std::holds_alternative<app::RemovedTorrent>(msg));
        const auto rt = std::get<app::RemovedTorrent>(msg);
        EXPECT_EQ(rt.infoHash, INFOHASH);
    }
}

TEST_F(BitTorrentManagerTest, processStopTorrent)
{
    btManReal.process(app::StopTorrent{INFOHASH});

    {
        ASSERT_TRUE(annResponses.numToRead());
        const auto msg = annResponses.pop();
        ASSERT_TRUE(std::holds_alternative<http::Pause>(msg));
        const auto ps = std::get<http::Pause>(msg);
        EXPECT_EQ(ps.infoHash, INFOHASH);
    }
    {
        ASSERT_TRUE(appResponses.numToRead());
        const auto msg = appResponses.pop();
        ASSERT_TRUE(std::holds_alternative<app::StoppedTorrent>(msg));
        const auto st = std::get<app::StoppedTorrent>(msg);
        EXPECT_EQ(st.infoHash, INFOHASH);
    }
}

TEST_F(BitTorrentManagerTest, processStartTorrent)
{
    btManReal.process(app::StartTorrent{INFOHASH, persist::TorrentModel{}, {}});
}

TEST_F(BitTorrentManagerTest, processShutdown)
{
    {
        EXPECT_CALL(peerService, shutdown()).Times(1);
    }

    btManReal.process(app::Shutdown{});

    {
        ASSERT_TRUE(annResponses.numToRead());
        const auto msg = annResponses.pop();
        ASSERT_TRUE(std::holds_alternative<http::Shutdown>(msg));
    }
    {
        ASSERT_TRUE(persistResponses.numToRead());
        const auto msg = persistResponses.pop();
        ASSERT_TRUE(std::holds_alternative<persist::Shutdown>(msg));
    }
    {
        ASSERT_TRUE(diskResponses.numToRead());
        const auto msg = diskResponses.pop();
        ASSERT_TRUE(std::holds_alternative<disk::Shutdown>(msg));
    }
    {
        ASSERT_TRUE(appResponses.numToRead());
        const auto msg = appResponses.pop();
        ASSERT_TRUE(std::holds_alternative<app::ShutdownConfirmation>(msg));
    }
}

TEST_F(BitTorrentManagerTest, processRequestStats)
{
    btManReal.process(app::StartTorrent{INFOHASH, persist::TorrentModel{}, {}});
    appResponses.pop();
    btManReal.process(app::StartTorrent{INFOHASH2, persist::TorrentModel{}, {}});
    appResponses.pop();

    btManReal.process(
        app::RequestStats{{std::make_pair(0, INFOHASH), std::make_pair(1, INFOHASH2)}});

    {
        ASSERT_EQ(appResponses.numToRead(), 2);
        const auto msg = appResponses.pop();
        ASSERT_TRUE(std::holds_alternative<app::PeerStats>(msg));
        const auto ps1 = std::get<app::PeerStats>(msg);
        EXPECT_EQ(ps1.connectedPeersCount, 0);
        EXPECT_EQ(ps1.knownPeerCount, 0);
        EXPECT_EQ(ps1.torrId, 0);

        const auto msg2 = appResponses.pop();
        ASSERT_TRUE(std::holds_alternative<app::PeerStats>(msg2));
        const auto ps2 = std::get<app::PeerStats>(msg2);
        EXPECT_EQ(ps2.connectedPeersCount, 0);
        EXPECT_EQ(ps2.knownPeerCount, 0);
        EXPECT_EQ(ps2.torrId, 1);
    }
}

} // namespace fractals::network::p2p