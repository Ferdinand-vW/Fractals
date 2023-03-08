#pragma once

#include "fractals/common/WorkQueue.h"
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

        uint32_t numToRead() const
        {   
            return popEnd.size();
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
    WorkQueueImpl<SIZE, RightEventIn> rightQueue;
};
} // namespace fractals::common