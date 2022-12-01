#pragma once

#include "Event.h"
#include "fractals/network/p2p/AnnounceService.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>

namespace fractals::network::p2p
{
    

    template <uint32_t SIZE, typename Event>
    class WorkQueueImpl
    {
        static constexpr uint32_t QUEUE_SIZE = SIZE;
        Event mEvents[QUEUE_SIZE];
        int32_t mHead{0};
        int32_t mTail{0};

        private:
            void calibrate()
            {
                const int32_t divisor = mTail / QUEUE_SIZE;
                const auto queueStart = QUEUE_SIZE * divisor;
                mHead = mHead - queueStart;
                mTail = mTail - queueStart;
            }

        public:
            WorkQueueImpl() = default;
            WorkQueueImpl(const WorkQueueImpl<SIZE, Event>&) = delete;
            WorkQueueImpl(WorkQueueImpl<SIZE, Event>&&) = delete;

            void push(Event&& event)
            {
                if (size() == QUEUE_SIZE)
                {
                    return;
                }
                mEvents[mHead % QUEUE_SIZE] = event;

                ++mHead;
            }

            bool isEmpty()
            {
                return size() == 0;
            }

            Event&& pop()
            {
                auto &&res = std::move(mEvents[mTail % QUEUE_SIZE]);
                ++mTail;

                calibrate();
                return std::move(res);
            }

            template<typename F>
            void forEach(F f)
            {
                for(auto ptr = mTail; ptr < mHead; ++ptr)
                {
                    f(mEvents[ptr]);   
                }
            }

            size_t size()
            {
                return mHead - mTail;
            }
    };

    static constexpr uint32_t WORK_QUEUE_SIZE = 256;
    using WorkQueue = WorkQueueImpl<WORK_QUEUE_SIZE, PeerEvent>;
}