#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/WorkQueue.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::StrictMock;
using namespace fractals::network::p2p;

struct Item
{
    int32_t n;
};

TEST(WORKQUEUE, AddRemoveItems)
{
    WorkQueueImpl<5, Item> queue;

    queue.push(Item{1});
    queue.push(Item{2});


    ASSERT_EQ(queue.size(), 2);
    auto i1 = queue.pop();
    ASSERT_EQ(queue.size(), 1);
    auto i2 = queue.pop();
    ASSERT_EQ(queue.size(), 0);

    ASSERT_EQ(i1.n, 1);
    ASSERT_EQ(i2.n, 2);
}

TEST(WORKQUEUE, QueueFull)
{
    WorkQueueImpl<5, Item> queue;

    queue.push(Item{1});
    queue.push(Item{2});
    queue.push(Item{3});
    queue.push(Item{4});
    queue.push(Item{5});
    ASSERT_EQ(queue.size(), 5);
    queue.push(Item{6});
    ASSERT_EQ(queue.size(), 5);
    queue.push(Item{7});
    ASSERT_EQ(queue.size(), 5);
}

TEST(WORKQUEUE, CIRCLE)
{
    WorkQueueImpl<5, Item> queue;

    queue.push(Item{0});
    queue.push(Item{1});
    queue.push(Item{2});
    queue.push(Item{3});
    queue.push(Item{4});

    for(int i = 5; i < 1000; i++)
    {
        queue.push(Item{i});
        ASSERT_EQ(queue.size(), 5);
    }
}

TEST(WORKQUEUE, CIRCLE_ONCE_THEN_CLEAR)
{
    WorkQueueImpl<5, Item> queue;

    queue.push(Item{0});
    queue.push(Item{1});
    queue.push(Item{2});
    queue.push(Item{3});
    queue.push(Item{4});
    queue.push(Item{5});
    queue.push(Item{6});
    queue.push(Item{7});

    queue.forEach([](auto item) {
        std::cout << item.n << std::endl;
    });

    ASSERT_EQ(queue.size(), 5);
    auto res1 = queue.pop();
    ASSERT_EQ(queue.size(), 4);
    ASSERT_EQ(res1.n, 3);
    auto res2 = queue.pop();
    ASSERT_EQ(queue.size(), 3);
    ASSERT_EQ(res2.n, 4);
    auto res3 = queue.pop();
    ASSERT_EQ(queue.size(), 2);
    ASSERT_EQ(res3.n, 5);
    auto res4 = queue.pop();
    ASSERT_EQ(queue.size(), 1);
    ASSERT_EQ(res4.n, 6);
    auto res5 = queue.pop();
    ASSERT_EQ(queue.size(), 0);
    ASSERT_EQ(res5.n, 7);

    queue.forEach([](auto item) {
        std::cout << item.n << std::endl;
    });

    auto res6 = queue.pop();
    queue.pop();
    queue.pop();
    auto res7 = queue.pop();
    ASSERT_EQ(res6.n, 2);
}

template <int MAX_SIZE>
void alternateTest()
{
    WorkQueueImpl<MAX_SIZE, Item> queue;

    int pushNum = 3;
    int pushMask = pushNum + 5;
    int popNum = 1;
    int popMask = popNum + 5;
    int maxIterations = 120;
    int iteration = 0;

    int currSize = 0;
    int pushed = 0;
    int popped = 0;
    while (iteration < maxIterations)
    {
        ASSERT_EQ(queue.size(), currSize);
        
        pushed = std::max(pushNum % pushMask, 3);
        for(int i = 0; i < pushed; i++)
        {
            queue.push(Item{i});
            currSize = std::min(currSize + 1, MAX_SIZE);
        }

        ASSERT_EQ(queue.size(), currSize);

        popped = std::max(pushed - 2, 1);
        for(int i = 0; i < popped; i++)
        {
            if(!queue.isEmpty())
            {
                queue.pop();
            }
            currSize = std::max(currSize - 1, 0);
        }

        ++iteration;
        ++pushNum;
        ++popNum;
    }
}

TEST(WORKQUEUE, alternate)
{
    alternateTest<4>();
    alternateTest<64>();
    alternateTest<500>();
    alternateTest<10000>();
}



