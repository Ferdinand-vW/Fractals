#include "fractals/network/p2p/PieceStateManager.h"

#include "gmock/gmock.h"
#include <cmath>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>

using namespace fractals::network::p2p;

TEST(PieceStateManager, construct)
{
    std::unordered_set<uint32_t> pieces{1,2,3,4,5};
    PieceStateManager psm(pieces, 123);

    // 1  2  3  4   5
    // 30 30 30 30  3
    // 30 60 90 120 123
    for(int i = 0; i < pieces.size() - 1; i++)
    {
        std::unordered_set<uint32_t>::iterator it = pieces.begin();
        std::advance(it, i);
        auto* ps = psm.getPieceState(*it);

        ASSERT_TRUE(ps);

        ASSERT_EQ(ps->getMaxSize(), 30);
    }

    std::unordered_set<uint32_t>::iterator it = pieces.begin();
    std::advance(it, pieces.size() - 1);
    auto * ps = psm.getPieceState(*it);

    ASSERT_TRUE(ps);

    ASSERT_EQ(ps->getMaxSize(), 3);
}

TEST(PieceStateManager, addBlock)
{
    PieceStateManager psm({1}, 10);

    psm.initializePiece(1);

    PieceState* ps = psm.getPieceState(1);

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



