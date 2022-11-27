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

            /*
            -1 0 | 0 - -1 - 1
            0 1 | 1 - 0 - 1
            1 2 | 2 - 1 - 1
            2 0 | 0 - 2
            0 1
            */

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
                std::cout << "-----S " << mHead << " " << mTail << " " << this->size() << std::endl;
                mEvents[mHead % QUEUE_SIZE] = event;

                // Condition implies that we will overwrite the tail event
                // Therefore the next last event is now the tail
                std::cout << "S " << mHead << " " << mTail << " " << this->size() << std::endl;
                ++mHead;
                std::cout << "S " << mHead << " " << mTail << " " << this->size() << std::endl;
                std::cout << "S " << mHead << " " << mTail << " " << this->size() << std::endl;
            }

            bool isEmpty()
            {
                return size() == 0;
            }

            Event&& pop()
            {
                std::cout << "X " << mHead << " " << mTail << " " << this->size() << std::endl;
                auto &&res = std::move(mEvents[mTail % QUEUE_SIZE]);
                ++mTail;
                calibrate();
                std::cout << "X " << mHead << " " << mTail << " " << this->size() << std::endl;
                return std::move(res);
            }

            template<typename F>
            void forEach(F f)
            {
                for(auto ptr = mTail; ptr < mHead; ++ptr)
                {
                    std::cout << "PTR " << ptr << std::endl;
                    f(mEvents[ptr]);   
                }
            }

            size_t size()
            {
                return mHead - mTail;

                if (mHead >= mTail)
                {
                    std::cout << "Size: " << mHead - std::max(mTail, 0) << std::endl;
                    return mHead - std::max(mTail, 0);
                }
                else
                {
                    std::cout << "Size: " << QUEUE_SIZE - mTail + mHead << std::endl;
                    return QUEUE_SIZE - mTail + mHead;
                }
            }
    };

    static constexpr uint32_t WORK_QUEUE_SIZE = 256;
    using WorkQueue = WorkQueueImpl<WORK_QUEUE_SIZE, PeerEvent>;
}