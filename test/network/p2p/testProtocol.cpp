#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/network/p2p/Protocol.h"
#include "fractals/persist/Event.h"

#include "gmock/gmock.h"
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include <variant>

using namespace fractals::network::p2p;
using namespace fractals;

std::unordered_map<uint32_t, std::vector<char>> testDataMap
{
  {0, {'a','b'} },
  {1, {'d','e'} },
  {2, {'f','g'} },
  {3, {'h','k'} },
  {4, {'q','w','e'} },  
};

const auto hash0 = common::hex_to_bytes("da23614e02469a0d7c7bd1bdab5c9c474b1904dc");
const auto hash1 = common::hex_to_bytes("600ccd1b71569232d01d110bc63e906beab04d8c");
const auto hash2 = common::hex_to_bytes("72f77e84ba0149b2af1051f1318128dccf60ab60");
const auto hash3 = common::hex_to_bytes("4fe0d24231b6309c90a78eeb8dc6ff2ca2d4cb85");
const auto hash4 = common::hex_to_bytes("056eafe7cf52220de2df36845b8ed170c67e23e3");

std::unordered_map<uint32_t, std::vector<char>> testHashDataMap
{
  {0, hash0 },
  {1, hash1 },
  {2, hash2 },
  {3, hash3 },
  {4, hash4 },  
};

class ProtocolTest : public ::testing::Test
{
    public:
    PieceStateManager pieceRepo{{0,1,2,3,4}, {}, 11, testHashDataMap};
    network::http::PeerId peer{"host",0};
    BitTorrentMsgQueue sendQueue;
    persist::StorageEventQueue storageQueue;
    disk::DiskEventQueue diskQueue;
    Protocol prot{Protocol(peer, sendQueue, storageQueue, diskQueue, pieceRepo)};
};

TEST_F(ProtocolTest, standardEval)
{
    {
        prot.onMessage(Choke{});

        ASSERT_TRUE(sendQueue.isEmpty());
        ASSERT_TRUE(storageQueue.isEmpty());
        ASSERT_TRUE(diskQueue.isEmpty());
    }

    {
        prot.onMessage(Bitfield(common::bitfield_to_bytes({1,1,1,1,1,0,0,0})));

        ASSERT_EQ(sendQueue.size(), 1);
        const auto msg1 = sendQueue.pop();
        ASSERT_TRUE(std::holds_alternative<Interested>(msg1));

        ASSERT_TRUE(storageQueue.isEmpty());
        ASSERT_TRUE(diskQueue.isEmpty());
    }

    prot.onMessage(UnChoke{});

    ASSERT_EQ(sendQueue.size(), 1);
    const auto msg2 = sendQueue.pop();
    ASSERT_TRUE(std::holds_alternative<Request>(msg2));
    auto reqPieceIndex = std::get<Request>(msg2).getReqIndex();
    auto reqPieceBegin = std::get<Request>(msg2).getReqBegin();    
    ASSERT_TRUE(storageQueue.isEmpty());
    ASSERT_TRUE(diskQueue.isEmpty());

    auto receiveAndRespond = [&]()
    {
        const auto data = testDataMap[reqPieceIndex];
        prot.onMessage(Piece(reqPieceIndex, reqPieceBegin, data));

        {
            ASSERT_EQ(storageQueue.size(), 1);
            const auto storageEvent1 = storageQueue.pop();

            ASSERT_TRUE(std::holds_alternative<persist::AddPieces>(storageEvent1));
            ASSERT_EQ(std::get<persist::AddPieces>(storageEvent1).mPieceIndex, reqPieceIndex);
        }

        {
            ASSERT_EQ(diskQueue.size(), 1);
            const auto diskEvent1 = diskQueue.pop();

            ASSERT_TRUE(std::holds_alternative<disk::WriteData>(diskEvent1));
            const auto diskEvent = std::get<disk::WriteData>(diskEvent1);
            
            ASSERT_EQ(diskEvent.mData.size(), data.size());
            for(int i = 0; i < data.size(); ++i)
            {
                ASSERT_EQ(diskEvent.mData[i], data[i]);
            }
            ASSERT_EQ(diskEvent.mPieceIndex, reqPieceIndex);
        }

        {
            ASSERT_EQ(sendQueue.size(), 1);
            const auto msg3 = sendQueue.pop();
            
            ASSERT_TRUE(std::holds_alternative<Request>(msg3));
            reqPieceIndex = std::get<Request>(msg3).getReqIndex();
            reqPieceBegin = std::get<Request>(msg3).getReqBegin();
        }
    };

    receiveAndRespond();
    receiveAndRespond();
    receiveAndRespond();
    receiveAndRespond();

    prot.onMessage(Piece(reqPieceIndex, reqPieceBegin, testDataMap[reqPieceIndex]));
        
    ASSERT_EQ(storageQueue.size(), 1);
    const auto storageEvent1 = storageQueue.pop();
    ASSERT_TRUE(std::holds_alternative<persist::AddPieces>(storageEvent1));
    ASSERT_EQ(std::get<persist::AddPieces>(storageEvent1).mPieceIndex, reqPieceIndex);
    
    ASSERT_EQ(diskQueue.size(), 1);
    const auto diskEvent1 = diskQueue.pop();
    ASSERT_TRUE(std::holds_alternative<disk::WriteData>(diskEvent1));
    EXPECT_THAT(std::get<disk::WriteData>(diskEvent1).mData, testDataMap[reqPieceIndex]);
    ASSERT_EQ(std::get<disk::WriteData>(diskEvent1).mPieceIndex, reqPieceIndex);

    ASSERT_EQ(sendQueue.size(), 0); // Nothing more to request
}

TEST_F(ProtocolTest, partial_pieces)
{
    {
        prot.onMessage(Have{0});

        ASSERT_EQ(sendQueue.size(), 1);

        const auto msg = sendQueue.pop();

        ASSERT_TRUE(std::holds_alternative<Interested>(msg));
    }

    {
        prot.onMessage(Piece(0, 0, std::vector<char>{'a'}));

        ASSERT_EQ(sendQueue.size(), 1);

        const auto msg = sendQueue.pop();
                
        ASSERT_TRUE(std::holds_alternative<Request>(msg));
        const auto reqPieceIndex = std::get<Request>(msg).getReqIndex();
        const auto reqPieceBegin = std::get<Request>(msg).getReqBegin();

        ASSERT_EQ(reqPieceIndex, 0);
        ASSERT_EQ(reqPieceBegin, 1);

        // Not enough data has been downloaded
        ASSERT_EQ(storageQueue.size(), 0);
        ASSERT_EQ(diskQueue.size(), 0);
    }

    {
        prot.onMessage(Piece(0, 1, std::vector<char>{'b'}));

        ASSERT_EQ(storageQueue.size(), 1);
        ASSERT_EQ(diskQueue.size(), 1);
        ASSERT_EQ(sendQueue.size(), 0);
    }
}

TEST_F(ProtocolTest, hash_check_fail)
{
    {
        prot.onMessage(Have{0});

        ASSERT_EQ(sendQueue.size(), 1);
        sendQueue.pop();
    }

    {
        const auto result = prot.onMessage(Piece(0, 0, std::vector<char>{'a','c'}));

        ASSERT_EQ(result, ProtocolState::HASH_CHECK_FAIL);
    }

    {
        const auto result = prot.onMessage(Piece(1, 0, std::vector<char>{'d','e'}));

        ASSERT_EQ(result, ProtocolState::OPEN);
    }
}


