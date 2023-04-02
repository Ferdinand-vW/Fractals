#include "Event.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/ConnectionEventHandler.h"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/Socket.h"

#include <bitset>
#include <cassert>
#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/Error.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <sys/eventfd.h>

namespace fractals::network::p2p
{
template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::ConnectionEventHandler(
    Epoll &epoll, BufferedQueueManagerT &bufMan)
    : mEpoll(epoll), mBufferedQueueManager(bufMan)
{
    mBufferedQueueManager.setWriteNotifier([this](const Peer &peer) { subscribe(peer); });
}

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
epoll_wrapper::CtlAction ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::unsubscribe(
    const Peer &peer)
{
    return mEpoll.erase(peer);
}

template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
bool ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::isSubscribed(const Peer &peer)
{
    return mEpoll.hasFd(peer.getFileDescriptor());
}

template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
int ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::readSocket(const Peer &peer)
{
    std::vector<char> buf(512);

    int rd = 0;
    while (int n = read(peer.getFileDescriptor(), &buf[0], 512))
    {
        if (n <= 0)
        {
            break;
        }

        rd += n;
        common::string_view view(buf.begin(), buf.begin() + n);
        mBufferedQueueManager.readPeerData(peer.getId(), view);
    }

    return rd;
}

template <typename Peer> void writeSocket(const Peer &peer, WriteMsgState *msgState)
{
    int n = write(peer.getFileDescriptor(), msgState->getBuffer().data(), msgState->remaining());
    msgState->flush(n);
}

template <ActionType Action, typename Peer, typename Epoll, typename BufferedQueueManagerT>
void ConnectionEventHandler<Action, Peer, Epoll, BufferedQueueManagerT>::run()
{
    mIsActive = true;

    while (mIsActive)
    {
        const auto wa = mEpoll.wait();

        if (wa.hasError())
        {
            spdlog::info("CEH::run wait has error {}", std::to_string(wa.getError()));
            mIsActive = false;
            mBufferedQueueManager.publishToQueue(EpollError{wa.getError()});
            return;
        }

        for (auto &[peer, event] : wa.getEvents())
        {
            if (mSpecialFd && mSpecialFd.value() == peer.getFileDescriptor())
            {
                mSpecialFd.reset();
                return;
            }

            if (isSubscribed(peer))
            {
                spdlog::info("CEH::run peer={} is subscribed. events={}", peer.getId().toString(),
                             epoll_wrapper::toEpollEvent(event.mEvents));

                if (event.mEvents & epoll_wrapper::EventCode::EpollErr)
                {
                    spdlog::info("CEH::run peer={} event=EpollErr", peer.getId().toString());
                    mBufferedQueueManager.publishToQueue(ConnectionError{peer.getId(), event.mError});

                    unsubscribe(peer);
                }

                if (event.mEvents & epoll_wrapper::EventCode::EpollHUp)
                {
                    spdlog::info("CEH::run peer={} event=EpollHUp", peer.getId().toString());
                    mBufferedQueueManager.publishToQueue(ConnectionCloseEvent{peer.getId()});

                    unsubscribe(peer);
                }

                if constexpr (Action == ActionType::READ)
                {
                    if (event.mEvents & epoll_wrapper::EventCode::EpollIn)
                    {
                        int rd = readSocket(peer);

                        spdlog::info("CEH::run peer={} event=EpollIn read={}", peer.getId().toString(), rd);
                    }
                }
                else
                {
                    if (event.mEvents & epoll_wrapper::EventCode::EpollOut)
                    {
                        spdlog::info("CEH::run peer={} event=EpollOut", peer.getId().toString());
                        auto *writeMsg = mBufferedQueueManager.getWriteBuffer(peer);
                        writeSocket(peer, writeMsg);

                        if (writeMsg->isComplete())
                        {
                            spdlog::info("CEH::run peer={} complete write. event={}", peer.getId().toString(),
                             epoll_wrapper::toEpollEvent(event.mEvents));
                            unsubscribe(peer);
                        }
                    }
                }
            }
            else
            {
                spdlog::error("CEH::run peer={} not subscribed", peer.getId().toString());
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
    http::PeerId pId{"", 0};
    auto peerfd = PeerFd{pId, Socket{fd}};
    subscribe(peerfd);
    eventfd_write(fd, 10);
}

} // namespace fractals::network::p2p