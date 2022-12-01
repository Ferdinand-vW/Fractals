#include "Event.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/ConnectionEventHandler.h"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/Socket.h"

#include <bitset>
#include <epoll_wrapper/Error.h>
#include <sstream>
#include <stdexcept>
#include <sys/eventfd.h>

namespace fractals::network::p2p
{
    template <ActionType Action, typename Peer, typename Epoll, typename RQ>
    ConnectionEventHandler<Action, Peer, Epoll, RQ>::ConnectionEventHandler(Epoll& epoll, RQ &rq)
        : mEpoll(epoll), mQueue(rq) {}

    
    template <ActionType Action, typename Peer, typename Epoll, typename RQ>
    typename epoll_wrapper::CtlAction ConnectionEventHandler<Action, Peer, Epoll, RQ>::subscribe(const Peer &peer)
    {
        if constexpr (Action == ActionType::READ)
        {
            return mEpoll.add(peer, epoll_wrapper::EventCode::EpollIn);
        }
        else
        {
            return mEpoll.add(peer, epoll_wrapper::EventCode::EpollOut);
        }
    }

    template <ActionType Action, typename Peer, typename Epoll, typename RQ>
    typename epoll_wrapper::CtlAction ConnectionEventHandler<Action, Peer, Epoll, RQ>::unsubscribe(const Peer &peer)
    {
        return mEpoll.erase(peer);
    }

    template <ActionType Action, typename Peer, typename Epoll, typename RQ>
    bool ConnectionEventHandler<Action, Peer, Epoll, RQ>::isSubscribed(const Peer &peer)
    {
        return mEpoll.hasFd(peer.getFileDescriptor());
    }

    std::deque<char> readFromSocket(int32_t fd)
    {
        std::deque<char> deq;
        std::vector<char> buf;
        buf.resize(512);

        while(int n = read(fd, &buf[0], 512))
        {
            if (n <= 0)
            {
                break;
            }

            deq.insert(deq.end(), buf.begin(), buf.begin() + n);
        }

        return deq;
    }

    template <ActionType Action, typename Peer, typename Epoll, typename RQ>
    void ConnectionEventHandler<Action, Peer, Epoll, RQ>::run()
    {
        mIsActive = true;

        while(mIsActive)
        {
            const auto wa = mEpoll.wait();

            if (wa.hasError())
            {
                mIsActive = false;
                mQueue.push(EpollError{wa.getError()});
                return;
            }

            for(auto &[peer, event] : wa.getEvents())
            {
                const auto p = http::Peer{"", http::PeerId{"",0}};
                if (mSpecialFd && mSpecialFd.value() == peer.getFileDescriptor())
                {
                    mSpecialFd.reset();
                    return;
                }

                if (isSubscribed(peer))
                {
                    if (event.mEvents & epoll_wrapper::EventCode::EpollErr)
                    {
                        mQueue.push(ConnectionError{p, event.mError});

                        unsubscribe(peer);
                        break;
                    }

                    if (event.mEvents & epoll_wrapper::EventCode::EpollHUp )
                    {
                        mQueue.push(ConnectionCloseEvent{p});

                        unsubscribe(peer);
                        break;
                    }

                    if constexpr (Action == ActionType::READ)
                    {
                        if (event.mEvents & epoll_wrapper::EventCode::EpollIn)
                        {
                            auto data = readFromSocket(peer.getFileDescriptor());
                            mQueue.push(ReceiveEvent{p,std::move(data)});
                        }
                        else
                        {
                            std::cout << "MISSED EVENT: " << event.mEvents << std::endl;
                        }
                    }
                    else
                    {
                        if (event.mEvents & epoll_wrapper::EventCode::EpollOut)
                        {
                            auto data = readFromSocket(peer.getFileDescriptor());
                            mQueue.push(ReceiveEvent{p,std::move(data)});
                        }
                        else
                        {
                            std::cout << "MISSED EVENT: " << event.mEvents << std::endl;
                        }
                    }
                }
            }
        }
    }

    template <ActionType Action, typename Peer, typename Epoll, typename RQ>
    void ConnectionEventHandler<Action, Peer, Epoll, RQ>::stop()
    {
        mIsActive = false;

        int fd = eventfd(0, EFD_NONBLOCK);
        mSpecialFd = fd;
        assert(fd > 0);
        auto peerfd = PeerFd{"",0,Socket{fd}};
        subscribe(peerfd);
        eventfd_write(fd, 10);
    }

}