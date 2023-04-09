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

    template <ActionType Action, typename Peer, typename EpollT, typename BufferedQueueManagerT>
    class EpollServiceImpl
    {
        public:
            using Epoll = EpollT;

            EpollServiceImpl(Epoll &epoll, BufferedQueueManagerT &rq);
            EpollServiceImpl(const EpollServiceImpl&) = delete;
            EpollServiceImpl(EpollServiceImpl&&) = delete;

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

    template <ActionType AT>
    using EpollService = EpollServiceImpl<AT, PeerFd, epoll_wrapper::Epoll<PeerFd>, BufferedQueueManager>;
}