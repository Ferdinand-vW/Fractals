#pragma once

#include "PeerFd.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"

#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/Error.h>
#include <epoll_wrapper/Event.h>

namespace fractals::network::p2p
{

template <typename Peer, typename EpollT, typename BufferedQueueManagerT, typename EpollMsgQueue>
class EpollServiceImpl
{
  public:
    enum class State
    {
        Active,
        Inactive,
        Deactivating
    };
    using Epoll = EpollT;

    EpollServiceImpl(Epoll &epoll, BufferedQueueManagerT &rq, EpollMsgQueue& queue);
    EpollServiceImpl(const EpollServiceImpl &) = delete;
    EpollServiceImpl(EpollServiceImpl &&) = delete;

    void notify();

    bool isSubscribed(const Peer &peer);

    void run();

    State getState() const;

    typename EpollMsgQueue::LeftEndPoint getCommQueue();

  private:
    void subscribe(const Peer &peer);
    void unsubscribe(const Peer &peer);
    epoll_wrapper::CtlAction disableWrite(const Peer &peer);
    epoll_wrapper::CtlAction enableWrite(const Peer &peer);
    State stop();

    PeerFd createNotifyFd();
    int readSocket(const Peer &peer);

    PeerFd mSpecialFd;
    State state{State::Inactive};

    std::mutex mMutex;
    Epoll &mEpoll;
    BufferedQueueManagerT &buffMan;
    EpollMsgQueue& queue;
    typename EpollMsgQueue::RightEndPoint commQueue;
};

using EpollService = EpollServiceImpl<PeerFd, epoll_wrapper::Epoll<PeerFd>, BufferedQueueManager, EpollMsgQueue>;

} // namespace fractals::network::p2p
