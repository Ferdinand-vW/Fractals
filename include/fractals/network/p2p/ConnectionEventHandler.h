#pragma once

#include "PeerFd.h"
#include "Event.h"
#include "fractals/network/p2p/WorkQueue.h"

#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/Error.h>
#include <epoll_wrapper/Event.h>

namespace fractals::network::p2p
{
    enum class ActionType {READ , WRITE};

    template <ActionType Action, typename Peer, typename Epoll, typename ReceiveQueue>
    class ConnectionEventHandler
    {
        public:
            ConnectionEventHandler(Epoll &epoll, ReceiveQueue &rq);
            ConnectionEventHandler(const ConnectionEventHandler&) = delete;
            ConnectionEventHandler(ConnectionEventHandler&&) = delete;

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

    using ConnectionReadHandler = ConnectionEventHandler<ActionType::READ, PeerFd, epoll_wrapper::Epoll<PeerFd>, WorkQueueImpl<256, PeerEvent>>;
    using ConnectionWriteHandler = ConnectionEventHandler<ActionType::WRITE, PeerFd, epoll_wrapper::Epoll<PeerFd>, WorkQueueImpl<256, PeerEvent>>;
}