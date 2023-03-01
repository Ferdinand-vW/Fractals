#pragma once

#include "fractals/common/WorkQueue.h"

namespace fractals::common
{
template <uint32_t SIZE, typename Event> class FullDuplexQueue
{
  private:
    using Queue = WorkQueueImpl<SIZE, Event>;

  public:
    struct QueueEndPoint
    {
      private:
        QueueEndPoint(Queue &pushEnd, Queue &popEnd) : pushEnd(pushEnd), popEnd(popEnd){};

      public:
        void push(Event &&event)
        {
            pushEnd.push(event);
        }

        Event &&pop()
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

        Queue &pushEnd;
        Queue &popEnd;
    };

  public:
    FullDuplexQueue() = default;

    QueueEndPoint &getEndA()
    {
        return QueueAPI(dirA, dirB);
    }

    QueueEndPoint &getEndB()
    {
        return QueueAPI(dirB, dirA);
    }

  private:
    Queue dirA;
    Queue dirB;
};
} // namespace fractals::common