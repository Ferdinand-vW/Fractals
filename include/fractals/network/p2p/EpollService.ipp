#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollService.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
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

#define TEMPLATE template <typename Peer, typename Epoll, typename BufferedQueueManagerT, typename EpollMsgQueue>
#define PREFIX EpollServiceImpl<Peer, Epoll, BufferedQueueManagerT, EpollMsgQueue>

TEMPLATE
PREFIX::EpollServiceImpl(Epoll &epoll, BufferedQueueManagerT &bufMan, typename EpollMsgQueue::RightEndPoint queue)
    : mEpoll(epoll), buffMan(bufMan), mSpecialFd(createNotifyFd()), queue(queue)
{
    // buffMan.setWriteNotifier([this](const Peer &peer) { subscribe(peer); });
}

TEMPLATE
void PREFIX::subscribe(const Peer &peer)
{
    std::unique_lock<std::mutex> _lock(mMutex);
    spdlog::info("has lock");
    const auto ctl = mEpoll.add(peer, epoll_wrapper::EventCode::EpollIn | epoll_wrapper::EventCode::EpollOut);
    spdlog::info("CEH::subscribe peer={} error={}", peer.getId().toString(), std::to_string(ctl.getError()));
    queue.push(CtlResponse{peer, std::to_string(ctl.getError())});
}

TEMPLATE
void PREFIX::unsubscribe(const Peer &peer)
{
    std::unique_lock<std::mutex> _lock(mMutex);
    const auto ctl = mEpoll.erase(peer);
    spdlog::info("CEH::unsubscribe peer={} error={}", peer.getId().toString(), std::to_string(ctl.getError()));
    queue.push(CtlResponse{peer, std::to_string(ctl.getError())});
}

TEMPLATE
epoll_wrapper::CtlAction PREFIX::disableWrite(const Peer &peer)
{
    std::unique_lock<std::mutex> _lock(mMutex);
    const auto ctl = mEpoll.mod(peer, epoll_wrapper::EventCode::EpollIn);

    spdlog::info("CEH::disableWrite peer={} error={}", peer.getId().toString(), std::to_string(ctl.getError()));
    return ctl;
}

TEMPLATE
epoll_wrapper::CtlAction PREFIX::enableWrite(const Peer &peer)
{
    std::unique_lock<std::mutex> _lock(mMutex);
    const auto ctl = mEpoll.mod(peer, epoll_wrapper::EventCode::EpollIn | epoll_wrapper::EventCode::EpollOut);
    spdlog::info("CEH::enableWrite peer={} error={}", peer.getId().toString(), std::to_string(ctl.getError()));
    return ctl;
}

TEMPLATE
void PREFIX::notify()
{
    spdlog::info("CEH::notify");
    struct epoll_event event;
    event.events = EPOLLOUT | EPOLLONESHOT;
    event.data.fd = mSpecialFd.getFileDescriptor();
    // Call underlying epoll system call directly without updating internal hashmap as this is thread safe
    mEpoll.getUnderlying().epoll_ctl(EPOLL_CTL_DEL, mSpecialFd.getFileDescriptor(), &event);
    mEpoll.getUnderlying().epoll_ctl(EPOLL_CTL_ADD, mSpecialFd.getFileDescriptor(), &event);
}

TEMPLATE
bool PREFIX::isSubscribed(const Peer &peer)
{
    return mEpoll.hasFd(peer.getFileDescriptor());
}

TEMPLATE
PeerFd PREFIX::createNotifyFd()
{
    int32_t fd = eventfd(0, EFD_NONBLOCK);
    assert(fd > 0);
    spdlog::info("CEH::createNotifyFd fd={}", fd);
    return PeerFd{http::PeerId{"localhost", 0}, fd};
}

TEMPLATE
int PREFIX::readMany(const Peer &peer)
{
    bool keepReading = true;
    int total = 0;
    do
    {
        auto [bytes, n] = readOnce(peer);

        if (n > 0)
        {
            writeToBuffer(peer, std::move(bytes), n);
        }

        keepReading = n > 0;
        total += n;
    }
    while(keepReading);

    return total;
}

TEMPLATE
std::tuple<std::vector<char>, int> PREFIX::readOnce(const Peer &peer)
{
    std::vector<char> buf(512);
    int n = read(peer.getFileDescriptor(), &buf[0], 512);
    return std::make_tuple(buf,n);
}

TEMPLATE
void PREFIX::writeToBuffer(const Peer &peer, std::vector<char>&& data, int bytes)
{
    spdlog::info("RESERVE: {}", bytes);
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
        while (queue.canPop())
        {
            const auto req = queue.pop();

            std::visit(common::overloaded{[&](Subscribe sub) {
                                              spdlog::info("sub");
                                              subscribe(sub.peer);
                                          },
                                          [&](UnSubscribe unsub) {
                                              spdlog::info("unsub");
                                              unsubscribe(unsub.peer);
                                          },
                                          [&](Deactivate stop) {
                                              spdlog::info("CEH::pop from queue: Deactivate");
                                              state = State::Inactive;
                                              queue.push(DeactivateResponse{});
                                          },
                                          [&](WriteEvent data) {
                                              spdlog::info("write");
                                              enableWrite(data.peer);
                                              buffMan.addToWriteBuffer(data.peer, std::move(data.message));
                                          }},
                       req);
        }

        spdlog::info("here");
        if (state != State::Active)
        {
            spdlog::info("break");
            break;
        }
        spdlog::info("CEH::wait");
        const auto wa = mEpoll.wait(500);
        spdlog::info("CEH::exit wait. ErrorCode={} NumEvents={}", std::to_string(wa.getError()),
                     std::to_string(wa.getEvents().size()));

        const auto hasError =
            (epoll_wrapper::ErrorCode::None != wa.getError() && epoll_wrapper::ErrorCode::Eintr != wa.getError());
        if (hasError)
        {
            spdlog::info("CEH::run wait has error {}", std::to_string(wa.getError()));
            state = State::Inactive;
            queue.push(EpollError{wa.getError()});
            return;
        }

        for (auto &[peer, event] : wa.getEvents())
        {
            if (peer == mSpecialFd)
            {
                spdlog::info("CEH::specialFd eventCodes={} error={}", std::to_string(event.mEvents),
                             std::to_string(event.mError));
                continue;
            }

            if (isSubscribed(peer))
            {
                spdlog::info("CEH::run peer={} is subscribed. events={}", peer.getId().toString(),
                             epoll_wrapper::toEpollEvent(event.mEvents));

                if (event.mEvents & epoll_wrapper::EventCode::EpollErr)
                {
                    spdlog::info("CEH::run peer={} event=EpollErr", peer.getId().toString());

                    queue.push(ConnectionError{peer.getId(), event.mError});

                    unsubscribe(peer);
                }

                if (event.mEvents & epoll_wrapper::EventCode::EpollHUp)
                {
                    spdlog::info("CEH::run peer={} event=EpollHUp", peer.getId().toString());
                    queue.push(ConnectionCloseEvent{peer.getId()});

                    unsubscribe(peer);
                }
                if (event.mEvents & epoll_wrapper::EventCode::EpollIn)
                {
                    int numBytes = readMany(peer);
                    spdlog::info("CEH::run peer={} event=EpollIn read={}", peer.getId().toString(), numBytes);
                }
                if (event.mEvents & epoll_wrapper::EventCode::EpollOut)
                {
                    spdlog::info("CEH::run peer={} event=EpollOut", peer.getId().toString());
                    auto *writeMsg = buffMan.getWriteBuffer(peer);
                    if (!writeMsg || writeMsg->isComplete())
                    {
                        continue;
                    }

                    if (!writeData(peer, writeMsg))
                    {
                        disableWrite(peer);
                        queue.push(WriteEventResponse{peer, "Could not write any data to peer"});
                    }

                    if (writeMsg->isComplete())
                    {
                        spdlog::info("CEH::run peer={} complete write. event={}", peer.getId().toString(),
                                     epoll_wrapper::toEpollEvent(event.mEvents));
                        queue.push(WriteEventResponse{peer, ""});
                        disableWrite(peer);
                    }
                }
            }
            else
            {
                spdlog::error("CEH::run peer={} not subscribed", peer.getId().toString());
            }
        }
    }

    state = State::Inactive;
}

TEMPLATE
typename PREFIX::State PREFIX::stop()
{
    spdlog::info("CEH::stop");
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