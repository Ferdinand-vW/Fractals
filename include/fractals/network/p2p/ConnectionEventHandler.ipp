#include "Event.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/ConnectionEventHandler.h"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/Socket.h"

#include <bitset>
#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/Error.h>
#include <sstream>
#include <stdexcept>
#include <sys/eventfd.h>

namespace fractals::network::p2p
{
    template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
    ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::ConnectionEventHandler(Epoll& epoll, BufferedQueueManagerT &bufMan)
        : mEpoll(epoll), mBufferedQueueManager(bufMan) {}

    
    template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
    epoll_wrapper::CtlAction ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::subscribe(const Peer &peer)
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

    template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
    epoll_wrapper::CtlAction ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::unsubscribe(const Peer &peer)
    {
        return mEpoll.erase(peer);
    }

    template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
    bool ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::isSubscribed(const Peer &peer)
    {
        return mEpoll.hasFd(peer.getFileDescriptor());
    }

    template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
    void ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::readSocket(const Peer &peer)
    {
        std::vector<char> buf(512);

        while(int n = read(peer.getFileDescriptor(), &buf[0], 512))
        {
            if (n <= 0)
            {
                break;
            }

            std::cout << "READ: " << n << std::endl;
            std::cout << "BUFSIZE " << buf.size() << std::endl;

            common::string_view view(buf.begin(), buf.begin() + n);
            mBufferedQueueManager.addToReadBuffers(peer.getId(), view);
        }
    }

    template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
    void ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::run()
    {
        mIsActive = true;

        while(mIsActive)
        {
            const auto wa = mEpoll.wait();

            if (wa.hasError())
            {
                mIsActive = false;
                mBufferedQueueManager.publishToQueue(EpollError{wa.getError()});
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
                        mBufferedQueueManager.publishToQueue(ConnectionError{p, event.mError});

                        unsubscribe(peer);
                        break;
                    }

                    if (event.mEvents & epoll_wrapper::EventCode::EpollHUp )
                    {
                        mBufferedQueueManager.publishToQueue(ConnectionCloseEvent{p});

                        unsubscribe(peer);
                        break;
                    }

                    if constexpr (Action == ActionType::READ)
                    {
                        if (event.mEvents & epoll_wrapper::EventCode::EpollIn)
                        {
                            readSocket(peer);
                        }
                        else
                        {
                            std::cout << "MISSED EVENT: " << event.mEvents << std::endl;
                        }
                    }
                    else
                    {
                    }
                }
            }
        }
    }

    template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
    void ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::stop()
    {
        mIsActive = false;

        int fd = eventfd(0, EFD_NONBLOCK);
        mSpecialFd = fd;
        assert(fd > 0);
        http::PeerId pId{"",0};
        auto peerfd = PeerFd{pId,Socket{fd}};
        subscribe(peerfd);
        eventfd_write(fd, 10);
    }

}