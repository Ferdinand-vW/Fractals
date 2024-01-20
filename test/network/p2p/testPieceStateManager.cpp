#include <fractals/network/p2p/PieceStateManager.h>

#include "gmock/gmock.h"
#include <cmath>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unordered_set>

namespace fractals::network::p2p
{

const auto hash0 = common::hexToBytes("da23614e02469a0d7c7bd1bdab5c9c474b1904dc");
const auto hash1 = common::hexToBytes("600ccd1b71569232d01d110bc63e906beab04d8c");
const auto hash2 = common::hexToBytes("72f77e84ba0149b2af1051f1318128dccf60ab60");
const auto hash3 = common::hexToBytes("4fe0d24231b6309c90a78eeb8dc6ff2ca2d4cb85");
const auto hash4 = common::hexToBytes("056eafe7cf52220de2df36845b8ed170c67e23e3");

std::vector<fractals::persist::PieceModel> testPieces{
    {0, 0, 0, 30, hash0, false}, {0, 0, 1, 30, hash1, false}, {0, 0, 2, 30, hash2, false},
    {0, 0, 3, 30, hash3, false}, {0, 0, 4, 3, hash4, false},
};

TEST(PieceStateManager, construct)
{
    PieceStateManager psm;
    psm.populate(testPieces);

    // 1  2  3  4   5
    // 30 30 30 30  3
    // 30 60 90 120 123
    for (int i = 0; i < testPieces.size() - 1; i++)
    {
        auto it = testPieces.begin();
        std::advance(it, i);
        auto *ps = psm.getPieceState(it->piece);

        ASSERT_TRUE(ps);

        ASSERT_EQ(ps->getMaxSize(), 30);
    }

    auto it = testPieces.begin();
    std::advance(it, testPieces.size() - 1);
    auto *ps = psm.getPieceState(it->piece);

    ASSERT_TRUE(ps);

    ASSERT_EQ(ps->getMaxSize(), 3);
}

TEST(PieceStateManager, addBlock)
{
    PieceStateManager psm;
    psm.populate(testPieces);

    auto ps = psm.nextAvailablePiece({1});

    ASSERT_TRUE(ps);
    ASSERT_EQ(ps->getMaxSize(), 30);
    ASSERT_EQ(ps->getRemainingSize(), 30);
    ASSERT_EQ(ps->getNextBeginIndex(), 0);
    ASSERT_FALSE(ps->isComplete());

    ps->addBlock("123"); // Added 3 chars

    ASSERT_EQ(ps->getMaxSize(), 30);
    ASSERT_EQ(ps->getRemainingSize(), 27);
    ASSERT_EQ(ps->getNextBeginIndex(), 3);
    ASSERT_FALSE(ps->isComplete());

    ps->addBlock("1234"); // Added 4 chars

    ASSERT_EQ(ps->getMaxSize(), 30);
    ASSERT_EQ(ps->getRemainingSize(), 23);
    ASSERT_EQ(ps->getNextBeginIndex(), 7);
    ASSERT_FALSE(ps->isComplete());

    ps->addBlock("12345678912345678111122"); // Added 23 chars

    ASSERT_EQ(ps->getMaxSize(), 30);
    ASSERT_EQ(ps->getRemainingSize(), 0);
    ASSERT_EQ(ps->getNextBeginIndex(), 30);
    ASSERT_TRUE(ps->isComplete());

    std::vector<char> buf(ps->getBuffer().begin(), ps->getBuffer().end());
    std::string strBuf(buf.begin(), buf.end());
    EXPECT_EQ(strBuf, "123123412345678912345678111122");
}

} // namespace fractals::network::p2p