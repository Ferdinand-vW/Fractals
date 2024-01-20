#include <fractals/AppId.h>
#include <fractals/common/Tagged.h>
#include <fractals/common/utils.h>
#include <fractals/disk/DiskEventQueue.h>
#include <fractals/network/http/Peer.h>
#include <fractals/network/p2p/BitTorrentMsg.h>
#include <fractals/network/p2p/PeerService.h>
#include <fractals/network/p2p/PieceStateManager.h>
#include <fractals/network/p2p/Protocol.ipp>
#include <fractals/persist/Event.h>
#include <fractals/persist/PersistEventQueue.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unordered_set>
#include <variant>

using namespace ::testing;

namespace fractals::network::p2p
{

std::unordered_map<uint32_t, std::vector<char>> testDataMap{
    {0, {'a', 'b'}}, {1, {'d', 'e'}}, {2, {'f', 'g'}}, {3, {'h', 'k'}}, {4, {'q', 'w', 'e'}},
};

const auto hash0 = common::hexToBytes("da23614e02469a0d7c7bd1bdab5c9c474b1904dc");
const auto hash1 = common::hexToBytes("600ccd1b71569232d01d110bc63e906beab04d8c");
const auto hash2 = common::hexToBytes("72f77e84ba0149b2af1051f1318128dccf60ab60");
const auto hash3 = common::hexToBytes("4fe0d24231b6309c90a78eeb8dc6ff2ca2d4cb85");
const auto hash4 = common::hexToBytes("056eafe7cf52220de2df36845b8ed170c67e23e3");

std::vector<fractals::persist::PieceModel> testHashDataMap{
    {0, 0, 0, 2, hash0, false}, {0, 0, 1, 2, hash1, false}, {0, 0, 2, 2, hash2, false},
    {0, 0, 3, 2, hash3, false}, {0, 0, 4, 3, hash4, false},
};

class MockPeerService
{
  public:
    MOCK_METHOD(void, write,
                (fractals::network::http::PeerId, BitTorrentMessage &&, std::chrono::nanoseconds));
};

class ProtocolTest : public ::testing::Test
{
  public:
    ProtocolTest()
    {
        pieceRepo.populate(testHashDataMap);
    }

    void SetUp() override
    {
        ::testing::Test::SetUp();
        now = std::chrono::system_clock::now().time_since_epoch();
        pieceRepo.setActive(true);
    }

  public:
    PieceStateManager pieceRepo;
    network::http::PeerId peer{"host", 0};
    MockPeerService peerService;
    disk::DiskEventQueue diskQueue;
    disk::DiskEventQueue::RightEndPoint diskQueueE{diskQueue.getRightEnd()};
    Protocol<MockPeerService> prot{Protocol(Fractals::initAppId(), peer, common::InfoHash{"test"},
                                            peerService, diskQueue.getLeftEnd(), pieceRepo)};
    std::chrono::nanoseconds now;
};

TEST_F(ProtocolTest, standardEval)
{
    {
        prot.onMessage(Choke{}, now);

        ASSERT_TRUE(diskQueueE.numToRead() == 0);
    }

    {
        EXPECT_CALL(peerService, write(_, Eq(Interested{}), _)).Times(1);
        prot.onMessage(Bitfield(common::bitfieldToBytes({1, 1, 1, 1, 1, 0, 0, 0})), now);

        ASSERT_TRUE(diskQueueE.numToRead() == 0);
    }

    uint32_t pieceIndex = 4;
    EXPECT_CALL(
        peerService,
        write(_, Eq(Request{pieceIndex, 0, static_cast<uint32_t>(testDataMap[pieceIndex].size())}),
              _))
        .Times(1);
    prot.onMessage(UnChoke{}, now);

    ASSERT_TRUE(diskQueueE.numToRead() == 0);

    auto receiveAndRespond = [&]()
    {
        const auto data = testDataMap[pieceIndex];

        EXPECT_CALL(peerService,
                    write(_,
                          Eq(Request{pieceIndex - 1, 0,
                                     static_cast<uint32_t>(testDataMap[pieceIndex - 1].size())}),
                          _))
            .Times(1);
        prot.onMessage(Piece(pieceIndex, 0, data), now);

        {
            ASSERT_EQ(diskQueueE.numToRead(), 1);
            const auto diskEvent1 = diskQueueE.pop();

            ASSERT_TRUE(std::holds_alternative<disk::WriteData>(diskEvent1));
            const auto diskEvent = std::get<disk::WriteData>(diskEvent1);

            ASSERT_EQ(diskEvent.mData.size(), data.size());
            for (int i = 0; i < data.size(); ++i)
            {
                ASSERT_EQ(diskEvent.mData[i], data[i]);
            }
            ASSERT_EQ(diskEvent.mPieceIndex, pieceIndex);
        }

        pieceIndex--;
    };

    receiveAndRespond();
    receiveAndRespond();
    receiveAndRespond();
    receiveAndRespond();

    EXPECT_CALL(peerService, write(_, _, _)).Times(0);
    prot.onMessage(Piece(0, 0, testDataMap[0]), now);

    ASSERT_EQ(diskQueueE.numToRead(), 1);
    const auto diskEvent1 = diskQueueE.pop();
    ASSERT_TRUE(std::holds_alternative<disk::WriteData>(diskEvent1));
    EXPECT_THAT(std::get<disk::WriteData>(diskEvent1).mData, testDataMap[0]);
    ASSERT_EQ(std::get<disk::WriteData>(diskEvent1).mPieceIndex, 0);
}

TEST_F(ProtocolTest, partial_pieces)
{
    {
        EXPECT_CALL(peerService, write(_, Eq(Interested{}), _)).Times(1);
        prot.onMessage(Have{0}, now);
        EXPECT_CALL(peerService, write(_, Eq(Request{0, 0, 2}), _)).Times(1);
        prot.onMessage(UnChoke{}, now);
    }

    {
        EXPECT_CALL(peerService, write(_, Eq(Request{0, 1, 1}), _)).Times(1);
        prot.onMessage(Piece(0, 0, std::vector<char>{'a'}), now);

        // Not enough data has been downloaded
        ASSERT_EQ(diskQueueE.numToRead(), 0);
    }

    {
        EXPECT_CALL(peerService, write(_, _, _)).Times(0);
        prot.onMessage(Piece(0, 1, std::vector<char>{'b'}), now);

        ASSERT_EQ(diskQueueE.numToRead(), 1);
    }
}

TEST_F(ProtocolTest, hash_check_fail)
{
    {
        EXPECT_CALL(peerService, write(_, Eq(Interested{}), _)).Times(1);
        prot.onMessage(Have{0}, now);
        prot.onMessage(Have{1}, now);
        EXPECT_CALL(peerService, write(_, Eq(Request{1, 0, 2}), _)).Times(1);
        prot.onMessage(UnChoke{}, now);
    }

    {
        EXPECT_CALL(peerService, write(_, _, _)).Times(0);
        const auto result = prot.onMessage(Piece(0, 0, std::vector<char>{'a', 'c'}), now);

        ASSERT_EQ(result, ProtocolState::HASH_CHECK_FAIL);
        Mock::VerifyAndClearExpectations(&peerService);
    }

    {
        EXPECT_CALL(peerService, write(_, Eq(Request{0, 0, 2}), _)).Times(1);
        const auto result = prot.onMessage(Piece(1, 0, std::vector<char>{'d', 'e'}), now);

        ASSERT_EQ(result, ProtocolState::OPEN);
    }
}

} // namespace fractals::network::p2p