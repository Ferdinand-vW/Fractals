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
            std::deque<char> readFromSocket(int32_t fd);

        public:
            ReceiverWorkerImpl(Epoll &epoll, ReceiveQueue &rq);
            ReceiverWorkerImpl(const ReceiverWorkerImpl&) = delete;
            ReceiverWorkerImpl(ReceiverWorkerImpl&&) = delete;

            epoll_wrapper::CtlAction subscribe(const Peer &peer);
            epoll_wrapper::CtlAction unsubscribe(const Peer &peer);

            bool isSubscribed(const Peer &peer);

            void run();

            void stop();

        private:

            std::optional<uint32_t> mSpecialFd;
            bool mIsActive{false};

            Epoll &mEpoll;
            ReceiveQueue &mQueue;
    };

    using ReceiverWorker = ReceiverWorkerImpl<PeerFd, epoll_wrapper::Epoll<PeerFd>, WorkQueueImpl<256, PeerEvent>>;
}