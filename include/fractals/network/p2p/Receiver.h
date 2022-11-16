#pragma once

#include "PeerFd.h"
#include "Event.h"
#include "fractals/network/p2p/WorkQueue.h"

#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/Error.h>
#include <epoll_wrapper/Event.h>

namespace fractals::network::p2p
{
    template <typename Peer, typename Epoll, typename ReceiveQueue>
    class ReceiverWorkerImpl
    {
        private:
            std::deque<char>&& readFromSocket(int32_t fd)
            {
                std::deque<char> deq;
                std::vector<char> buf;
                buf.reserve(512);

                int count = 0;
                while(count >= 0)
                {
                    count = read(fd, &buf[0], buf.capacity());

                    if (count > 0)
                    {
                        deq.emplace_back(std::begin(buf), std::end(buf));
                    }
                }

                return std::move(deq);
            }

        public:
            ReceiverWorkerImpl(Epoll &epoll, ReceiveQueue &rq);
            ReceiverWorkerImpl(const ReceiverWorkerImpl&) = delete;
            ReceiverWorkerImpl(ReceiverWorkerImpl&&) = delete;

            epoll_wrapper::CtlAction subscribe(const Peer &peer);
            epoll_wrapper::CtlAction unsubscribe(const Peer &peer);

            void run();

            void stop();

        private:

            bool mIsActive{false};

            Epoll &mEpoll;
            ReceiveQueue &mQueue;
    };

    using ReceiverWorker = ReceiverWorkerImpl<PeerFd, epoll_wrapper::Epoll<PeerFd>, WorkQueueImpl<256, PeerEvent>>;
}