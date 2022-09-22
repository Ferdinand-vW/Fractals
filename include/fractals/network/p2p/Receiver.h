#pragma once

#include "PeerFd.h"

#include <epoll_wrapper/Epoll.h>

namespace fractals::network::p2p
{
    using ReceiverWorker = ReceiverWorkerImpl<PeerFd, epoll_wrapper::Epoll, ReceiveQueue<Peer>>;

    template <typename Peer, typename Epoll, template <typename> typename ReceiveQueue>
    class ReceiverWorkerImpl
    {
        public:
            ReceiverWorkerImpl(Epoll &epoll, ReceiveQueue<Peer> &rq);
            ReceiverWorkerImpl(const ReceiverWorkerImpl&) = delete;
            ReceiverWorkerImpl(ReceiverWorkerImpl&&) = delete;

            typename Epoll::Error subscribe(const std::unique_ptr<Peer> &peer);

            typename Epoll::Error unsubscribe(const std::unique_ptr<Peer> &peer);

            void run();

            void stop();

        private:

            bool mIsActive{false};

            Epoll &mEpoll;
            ReceiveQueue<Peer> &mQueue;
    };
}