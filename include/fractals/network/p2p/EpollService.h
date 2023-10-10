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

    EpollServiceImpl(Epoll &epoll, BufferedQueueManagerT &rq, typename EpollMsgQueue::RightEndPoint queue);
    EpollServiceImpl(const EpollServiceImpl &) = delete;
    EpollServiceImpl(EpollServiceImpl &&) = delete;

    void notify();

    bool isSubscribed(const Peer &peer);

    void run();

    State getState() const;
    bool isActive() const;

  private:
    void subscribe(const Peer &peer);
    void unsubscribe(const Peer &peer);
    epoll_wrapper::CtlAction disableWrite(const Peer &peer);
    epoll_wrapper::CtlAction enableWrite(const Peer &peer);
    State stop();

    PeerFd createNotifyFd();
    int readMany(const Peer &peer);
    std::tuple<std::vector<char>, int> readOnce(const Peer &peer);
    void writeToBuffer(const Peer& peer, std::vector<char>&& data, int bytes);

    int notifyPipe[2];
    PeerFd notifyPeer;

    State state{State::Inactive};
    std::unordered_set<PeerFd> pending;

    std::mutex mMutex;
    Epoll &mEpoll;
    BufferedQueueManagerT &buffMan;
    typename EpollMsgQueue::RightEndPoint queue;
};

using EpollService = EpollServiceImpl<PeerFd, epoll_wrapper::Epoll<PeerFd>, BufferedQueueManager, EpollMsgQueue>;

} // namespace fractals::network::p2p
