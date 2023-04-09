#pragma once

#include <array>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>

namespace fractals::common
{
template <uint32_t SIZE, typename Event> class WorkQueueImpl
{
    static constexpr uint32_t QUEUE_SIZE = SIZE;
    std::array<Event, QUEUE_SIZE> mEvents;
    int32_t mHead{0};
    int32_t mTail{0};
    std::condition_variable* cv{nullptr};

  private:
    void calibrate()
    {
        const int32_t divisor = mTail / QUEUE_SIZE;
        const auto queueStart = QUEUE_SIZE * divisor;
        mHead = mHead - queueStart;
        mTail = mTail - queueStart;
    }

  public:
    WorkQueueImpl(){};

    WorkQueueImpl(const WorkQueueImpl<SIZE, Event> &) = delete;
    WorkQueueImpl(WorkQueueImpl<SIZE, Event> &&) = delete;

    void push(Event &&event)
    {
        if (size() == QUEUE_SIZE)
        {
            return;
        }
        mEvents[mHead % QUEUE_SIZE] = event;

        ++mHead;

        notify();
    }

    bool isEmpty()
    {
        return size() == 0;
    }

    Event &&pop()
    {
        auto &&res = std::move(mEvents[mTail % QUEUE_SIZE]);
        
        calibrate();
        ++mTail;

        return std::move(res);
    }

    template <typename F> void forEach(F f)
    {
        for (auto ptr = mTail; ptr < mHead; ++ptr)
        {
            f(mEvents[ptr]);
        }
    }

    size_t size()
    {
        return mHead - mTail;
    }

    void attachNotifier(std::condition_variable* cv)
    {
        this->cv = cv;
    }

    void notify()
    {
        if (cv)
        {
            cv->notify_all();
        }
    }
};
} // namespace fractals::common