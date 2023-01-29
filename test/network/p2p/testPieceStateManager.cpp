#include "fractals/network/p2p/PieceStateManager.h"

#include "gmock/gmock.h"
#include <cmath>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>

using namespace fractals::common;

using namespace fractals::network::p2p;

const auto hash0 = hex_to_bytes("da23614e02469a0d7c7bd1bdab5c9c474b1904dc");
const auto hash1 = hex_to_bytes("600ccd1b71569232d01d110bc63e906beab04d8c");
const auto hash2 = hex_to_bytes("72f77e84ba0149b2af1051f1318128dccf60ab60");
const auto hash3 = hex_to_bytes("4fe0d24231b6309c90a78eeb8dc6ff2ca2d4cb85");
const auto hash4 = hex_to_bytes("056eafe7cf52220de2df36845b8ed170c67e23e3");

std::unordered_map<uint32_t, std::vector<char>> testHashDataMap
{
  {0, hash0 },
  {1, hash1 },
  {2, hash2 },
  {3, hash3 },
  {4, hash4 },  
};

TEST(PieceStateManager, construct)
{
    std::vector<uint32_t> pieces{1,2,3,4,5};
    PieceStateManager psm(pieces, {}, 123, testHashDataMap);

    // 1  2  3  4   5
    // 30 30 30 30  3
    // 30 60 90 120 123
    for(int i = 0; i < pieces.size() - 1; i++)
    {
        std::vector<uint32_t>::iterator it = pieces.begin();
        std::advance(it, i);
        auto* ps = psm.getPieceState(*it);

        ASSERT_TRUE(ps);

        ASSERT_EQ(ps->getMaxSize(), 30);
    }

    std::vector<uint32_t>::iterator it = pieces.begin();
    std::advance(it, pieces.size() - 1);
    auto * ps = psm.getPieceState(*it);

    ASSERT_TRUE(ps);

    ASSERT_EQ(ps->getMaxSize(), 3);
}

TEST(PieceStateManager, addBlock)
{
    PieceStateManager psm({1}, {}, 10, testHashDataMap);

    auto ps = psm.nextAvailablePiece({1});

    ASSERT_TRUE(ps);
    ASSERT_EQ(ps->getMaxSize(), 10);
    ASSERT_EQ(ps->getRemainingSize(), 10);
    ASSERT_EQ(ps->getNextBeginIndex(), 0);
    ASSERT_FALSE(ps->isComplete());

    ps->addBlock({'1','2','3'}); //Added 3 chars

    ASSERT_EQ(ps->getMaxSize(), 10);
    ASSERT_EQ(ps->getRemainingSize(), 7);
    ASSERT_EQ(ps->getNextBeginIndex(), 3);
    ASSERT_FALSE(ps->isComplete());

    ps->addBlock({'1','2','3', '4'}); //Added 3 chars

    ASSERT_EQ(ps->getMaxSize(), 10);
    ASSERT_EQ(ps->getRemainingSize(), 3);
    ASSERT_EQ(ps->getNextBeginIndex(), 7);
    ASSERT_FALSE(ps->isComplete());

    ps->addBlock({'1','2','3'}); //Added 3 chars

    ASSERT_EQ(ps->getMaxSize(), 10);
    ASSERT_EQ(ps->getRemainingSize(), 0);
    ASSERT_EQ(ps->getNextBeginIndex(), 10);
    ASSERT_TRUE(ps->isComplete());

    std::vector<char> buf(ps->getBuffer().begin(), ps->getBuffer().end());
    EXPECT_THAT(buf, testing::ContainerEq<std::vector<char>>({'1','2','3','1','2','3','4','1','2','3'}));
}



