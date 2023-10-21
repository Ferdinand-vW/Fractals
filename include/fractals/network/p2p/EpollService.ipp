#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollService.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/PeerEvent.h"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/Socket.h"

#include <bitset>
#include <cassert>
#include <chrono>
#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/EpollImpl.h>
#include <epoll_wrapper/Error.h>
#include <epoll_wrapper/Event.h>
#include <mutex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/eventfd.h>

namespace fractals::network::p2p
{

#define TEMPLATE                                                                                   \
    template <typename Peer, typename Epoll, typename BufferedQueueManagerT, typename EpollMsgQueue>
#define PREFIX EpollServiceImpl<Peer, Epoll, BufferedQueueManagerT, EpollMsgQueue>

TEMPLATE
PREFIX::EpollServiceImpl(Epoll &epoll, BufferedQueueManagerT &bufMan,
                         typename EpollMsgQueue::RightEndPoint queue)
    : mEpoll(epoll), buffMan(bufMan), notifyPeer(createNotifyFd()), queue(queue)
{
    mEpoll.add(notifyPeer, epoll_wrapper::EventCode::EpollIn);
}

TEMPLATE
void PREFIX::subscribe(const Peer &peer)
{
    const auto ctl =
        mEpoll.add(peer, epoll_wrapper::EventCode::EpollIn | epoll_wrapper::EventCode::EpollOut);
    queue.push(CtlResponse{peer, std::to_string(ctl.getError())});
}

TEMPLATE
void PREFIX::unsubscribe(const Peer &peer)
{
    pending.erase(peer); // may not be known to peer
    const auto pr = peer;
    const auto ctl = mEpoll.erase(peer);
    close(peer.getFileDescriptor());
    queue.push(CtlResponse{pr, std::to_string(ctl.getError())});
}

TEMPLATE
epoll_wrapper::CtlAction PREFIX::disableWrite(const Peer &peer)
{
    return mEpoll.mod(peer, epoll_wrapper::EventCode::EpollIn);
}

TEMPLATE
epoll_wrapper::CtlAction PREFIX::enableWrite(const Peer &peer)
{
    return mEpoll.mod(peer, epoll_wrapper::EventCode::EpollIn | epoll_wrapper::EventCode::EpollOut);
}

TEMPLATE
void PREFIX::notify()
{
    uint8_t buf[1]{0};
    write(notifyPipe[1], buf, 1);
}

TEMPLATE
bool PREFIX::isSubscribed(const Peer &peer)
{
    return mEpoll.hasFd(peer.getFileDescriptor());
}

TEMPLATE
PeerFd PREFIX::createNotifyFd()
{
    pipe(notifyPipe);

    int32_t fd = eventfd(0, EFD_NONBLOCK);
    assert(fd > 0);
    return PeerFd{http::PeerId{"", 0}, notifyPipe[0]};
}

TEMPLATE
int PREFIX::readMany(const Peer &peer)
{
    auto [bytes, n] = readOnce(peer);

    writeToBuffer(peer, std::move(bytes), n);

    return n;
}

TEMPLATE
std::tuple<std::vector<char>, int> PREFIX::readOnce(const Peer &peer)
{
    std::vector<char> buf(16 * 1024);
    int n = read(peer.getFileDescriptor(), &buf[0], 16 * 1024);
    if (n < 0)
    {
        spdlog::error("Error reading less than 0 bytes {}", strerror(errno));
    }
    return std::make_tuple(buf, n);
}

TEMPLATE
void PREFIX::writeToBuffer(const Peer &peer, std::vector<char> &&data, int bytes)
{
    std::vector<char> v(data.begin(), data.begin() + bytes);
    bool isComplete = buffMan.addToReadBuffer(peer, v);
    if (isComplete)
    {
        auto &msgs = buffMan.getReadBuffers(peer);
        for (auto it = msgs.begin(); it != msgs.end();)
        {
            if (it->isComplete())
            {
                queue.push(ReadEvent{peer, std::move(it->flush())});
                it = msgs.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

template <typename Peer> int writeData(const Peer &peer, WriteMsgState *msgState)
{
    int n = write(peer.getFileDescriptor(), msgState->getBuffer().data(), msgState->remaining());
    msgState->flush(n);

    return n;
}

TEMPLATE
void PREFIX::run()
{
    state = State::Active;

    while (state == State::Active)
    {
        const auto wa = mEpoll.wait();

        while (queue.canPop())
        {
            const auto req = queue.pop();

            std::visit(common::overloaded{[&](Subscribe sub)
                                          {
                                              subscribe(sub.peer);
                                              pending.emplace(sub.peer);
                                          },
                                          [&](UnSubscribe unsub)
                                          {
                                              unsubscribe(unsub.peer);
                                              queue.push(ConnectionCloseEvent{unsub.peer.getId()});
                                          },
                                          [&](Deactivate stop)
                                          {
                                              state = State::Inactive;
                                          },
                                          [&](WriteEvent data)
                                          {
                                              enableWrite(data.peer);
                                              buffMan.addToWriteBuffer(data.peer,
                                                                       std::move(data.message));
                                          }},
                       req);
        }

        if (state != State::Active)
        {
            spdlog::warn("EpollSerivce::run. Not active. Exiting.");
            break;
        }

        const auto hasError = (epoll_wrapper::ErrorCode::None != wa.getError() &&
                               epoll_wrapper::ErrorCode::Eintr != wa.getError());
        if (hasError)
        {
            state = State::Inactive;
            spdlog::error("EpollService::run error={}", std::to_string(wa.getError()));
            queue.push(EpollError{wa.getError()});
            return;
        }

        for (auto &[peer, event] : wa.getEvents())
        {
            if (peer == notifyPeer)
            {
                uint8_t buf[1];
                read(notifyPeer.getFileDescriptor(), buf, 1);
                continue;
            }

            if (isSubscribed(peer))
            {
                if (event.mEvents & epoll_wrapper::EventCode::EpollErr)
                {
                    queue.push(ConnectionError{peer.getId(), event.mError});

                    unsubscribe(peer);
                    continue;
                }

                if (event.mEvents & epoll_wrapper::EventCode::EpollHUp)
                {
                    queue.push(ConnectionCloseEvent{peer.getId()});

                    unsubscribe(peer);
                    continue;
                }

                if (event.mEvents &
                    (epoll_wrapper::EventCode::EpollIn | epoll_wrapper::EventCode::EpollPri))
                {
                    if (!readMany(peer))
                    {
                        queue.push(ConnectionCloseEvent{peer.getId()});
                        unsubscribe(peer);
                        continue;
                    }
                }
                if (event.mEvents & epoll_wrapper::EventCode::EpollOut)
                {
                    if (pending.count(peer))
                    {
                        queue.push(ConnectionAccepted{peer});
                        pending.erase(peer);
                    }

                    auto *writeMsg = buffMan.getWriteBuffer(peer);
                    if (!writeMsg)
                    {
                        spdlog::error("EpollService::run. could not find write buffer for peer={}",
                                      peer.getId().toString());
                        disableWrite(peer);
                        continue;
                    }

                    if (!writeData(peer, writeMsg))
                    {
                        queue.push(WriteEventResponse{peer, "Could not write any data to peer"});
                    }

                    if (writeMsg->isComplete())
                    {
                        queue.push(WriteEventResponse{peer, ""});
                        buffMan.removeFromWriteBuffer(peer);
                    }

                    disableWrite(peer);
                }
            }
            else
            {
                spdlog::error("EpollService::run. Received epoll event for not subscribed peer={}",
                              peer.getId().toString());
            }
        }
    }

    spdlog::info("EpollService::run. Shutdown");
}

TEMPLATE
typename PREFIX::State PREFIX::stop()
{
    spdlog::info("EpollService::stop");
    if (state == State::Active)
    {
        const auto newState = State::Deactivating;
        state = newState;

        notify();
        return newState;
    }

    return state;
}

TEMPLATE
typename PREFIX::State PREFIX::getState() const
{
    return state;
}

TEMPLATE
bool PREFIX::isActive() const
{
    return state == PREFIX::State::Active;
}

#undef TEMPLATE
#undef PREFIX

} // namespace fractals::network::p2p