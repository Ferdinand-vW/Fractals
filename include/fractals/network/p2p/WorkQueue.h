#pragma once

#include "Event.h"
#include "fractals/network/p2p/AnnounceService.h"

#include <cstdint>
#include <iostream>
#include <optional>

namespace fractals::network::p2p
{
    

    template <uint32_t SIZE, typename Event>
    class WorkQueueImpl
    {
        static constexpr uint32_t QUEUE_SIZE = SIZE + 1;
        Event mEvents[QUEUE_SIZE];
        int32_t mHead{0};
        int32_t mTail{-1};

        private:
            int32_t inc(int32_t ptr)
            {
                return (ptr + 1) % QUEUE_SIZE;
            }

        public:
            WorkQueueImpl() = default;
            WorkQueueImpl(const WorkQueueImpl<SIZE, Event>&) = delete;
            WorkQueueImpl(WorkQueueImpl<SIZE, Event>&&) = delete;

            void push(Event&& event)
            {
                mEvents[mHead] = event;

                // Condition implies that we will overwrite the tail event
                // Therefore the next last event is now the tail
                const auto nextHead = inc(mHead);
                if (nextHead == mTail || mTail < 0)
                {
                    mTail = inc(mTail);
                }

                mHead = nextHead;
                std::cout << "S " << mHead << " " << mTail << " " << this->size() << std::endl;
            }

            bool isEmpty()
            {
                return size() == 0;
            }

            Event&& pop()
            {
                std::cout << "X " << mHead << " " << mTail << " " << this->size() << std::endl;
                auto &&res = std::move(mEvents[mTail]);
                mTail = inc(mTail);

                std::cout << "X " << mHead << " " << mTail << " " << this->size() << std::endl;
                return std::move(res);
            }

            template<typename F>
            void forEach(F f)
            {
                auto ptr = mTail;
                while(ptr != mHead)
                {
                    std::cout << "PTR " << ptr << std::endl;
                    f(mEvents[ptr]);
                    
                    ptr = inc(ptr);
                }
            }

            size_t size()
            {
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