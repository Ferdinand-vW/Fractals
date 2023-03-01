#pragma once

#include "fractals/common/WorkQueue.h"

namespace fractals::common
{
template <uint32_t SIZE, typename LeftEventIn, typename RightEventIn> class FullDuplexQueue
{
  private:
    using QueueA = WorkQueueImpl<SIZE, LeftEventIn>;
    using QueueB = WorkQueueImpl<SIZE, RightEventIn>;

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
            return popEnd.pop();
        }

        bool canPop()
        {
            return !popEnd.isEmpty();
        }

        bool canPush()
        {
            return !pushEnd.size() == SIZE;
        }

      private:
        WorkQueueImpl<SIZE, PushEvent> &pushEnd;
        WorkQueueImpl<SIZE, PopEvent> &popEnd;
    };

  public:
    using LeftEndPoint = QueueEndPoint<LeftEventIn, RightEventIn>;
    using RightEndPoint = QueueEndPoint<RightEventIn, LeftEventIn>;

    FullDuplexQueue() = default;

    QueueEndPoint<LeftEventIn, RightEventIn> getLeftEnd()
    {
        return QueueEndPoint<LeftEventIn, RightEventIn>(dirA, dirB);
    }

    QueueEndPoint<RightEventIn, LeftEventIn> getRightEnd()
    {
        return QueueEndPoint<RightEventIn, LeftEventIn>(dirB, dirA);
    }

  private:
    WorkQueueImpl<SIZE, LeftEventIn> dirA;
    WorkQueueImpl<SIZE, RightEventIn> dirB;
};
} // namespace fractals::common