#pragma once

#include <fractals/common/WorkQueue.h>
#include <condition_variable>
#include <cstdint>

namespace fractals::common
{
template <uint32_t SIZE, typename LeftEventIn, typename RightEventIn> class FullDuplexQueue
{
  public:
    template <typename PushEvent, typename PopEvent> struct QueueEndPoint
    {
      public:
        QueueEndPoint(WorkQueueImpl<SIZE, PushEvent> &pushEnd, WorkQueueImpl<SIZE, PopEvent> &popEnd)
            : pushEnd(pushEnd), popEnd(popEnd){};

        void push(PushEvent &&event)
        {
            pushEnd.push(std::move(event));
        }

        PopEvent &&pop()
        {
            return std::move(popEnd.pop());
        }

        bool canPop() const
        {
            return !popEnd.isEmpty();
        }

        bool canPush() const
        {
            return !pushEnd.size() == SIZE;
        }

        uint32_t numToRead() const
        {
            return popEnd.size();
        }

        void attachNotifier(std::mutex& mutex, std::condition_variable &cv)
        {
            popEnd.attachNotifier(mutex, cv);
        }

        void notify()
        {
            pushEnd.notify();
        }

      private:
        WorkQueueImpl<SIZE, PushEvent> &pushEnd;
        WorkQueueImpl<SIZE, PopEvent> &popEnd;
    };

  public:
    using LeftEndPoint = QueueEndPoint<LeftEventIn, RightEventIn>;
    using RightEndPoint = QueueEndPoint<RightEventIn, LeftEventIn>;

    FullDuplexQueue(){};

    QueueEndPoint<LeftEventIn, RightEventIn> getLeftEnd()
    {
        return QueueEndPoint<LeftEventIn, RightEventIn>(leftQueue, rightQueue);
    }

    QueueEndPoint<RightEventIn, LeftEventIn> getRightEnd()
    {
        return QueueEndPoint<RightEventIn, LeftEventIn>(rightQueue, leftQueue);
    }

  private:
    WorkQueueImpl<SIZE, LeftEventIn> leftQueue;
    std::mutex leftMutex;
    std::condition_variable leftCv;
    WorkQueueImpl<SIZE, RightEventIn> rightQueue;
    std::mutex rightMutex;
    std::condition_variable rightCv;
};
} // namespace fractals::common