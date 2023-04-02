#pragma once

#include "PeerFd.h"
#include "Event.h"
#include "fractals/network/p2p/BufferedQueueManager.h"

#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/Error.h>
#include <epoll_wrapper/Event.h>

namespace fractals::network::p2p
{
    enum class ActionType {READ , WRITE};

    template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
    class ConnectionEventHandler
    {
        public:
            ConnectionEventHandler(Epoll &epoll, BufferedQueueManagerT &rq);
            ConnectionEventHandler(const ConnectionEventHandler&) = delete;
            ConnectionEventHandler(ConnectionEventHandler&&) = delete;

            epoll_wrapper::CtlAction subscribe(const Peer &peer);
            epoll_wrapper::CtlAction unsubscribe(const Peer &peer);

            bool isSubscribed(const Peer &peer);

            void run();

            void stop();

        private:
            int readSocket(const Peer &peer);

            std::optional<uint32_t> mSpecialFd;
            bool mIsActive{false};

            Epoll &mEpoll;
            BufferedQueueManagerT &mBufferedQueueManager;
    };

    using ConnectionReadHandler = ConnectionEventHandler<ActionType::READ, PeerFd, epoll_wrapper::Epoll<PeerFd>, BufferedQueueManager>;
    using ConnectionWriteHandler = ConnectionEventHandler<ActionType::WRITE, PeerFd, epoll_wrapper::Epoll<PeerFd>, BufferedQueueManager>;
}