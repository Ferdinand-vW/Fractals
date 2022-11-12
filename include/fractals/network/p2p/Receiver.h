#pragma once

#include "PeerFd.h"
#include "Event.h"
#include "fractals/network/p2p/WorkQueue.h"

#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/Error.h>
#include <epoll_wrapper/Event.h>

namespace fractals::network::p2p
{
    template <typename Peer, template <typename> typename Epoll, typename ReceiveQueue>
    class ReceiverWorkerImpl
    {
        private:
            std::deque<char>&& readFromSocket(int32_t fd)
            {
                std::deque<char> deq;
                std::vector<char> buf;
                buf.reserve(512);

                int count = 0;
                while(count >= 0)
                {
                    count = read(fd, &buf[0], buf.capacity());

                    if (count > 0)
                    {
                        deq.emplace_back(std::begin(buf), std::end(buf));
                    }
                }

                return std::move(deq);
            }

        public:
            ReceiverWorkerImpl(Epoll<Peer> &epoll, ReceiveQueue &rq);
            ReceiverWorkerImpl(const ReceiverWorkerImpl&) = delete;
            ReceiverWorkerImpl(ReceiverWorkerImpl&&) = delete;

            typename Epoll<Peer>::CtlAction subscribe(const std::unique_ptr<Peer> &peer)
            {
                return mEpoll.add(peer, epoll_wrapper::EventCode::EpollOut);
            }

            typename Epoll<Peer>::CtlAction unsubscribe(const std::unique_ptr<Peer> &peer)
            {
                return mEpoll.erase(peer);
            }

            void run()
            {
                mIsActive = true;

                while(mIsActive)
                {
                    const auto wa = mEpoll.wait();

                    if (wa.hasError())
                    {
                        mIsActive = false;
                        mQueue.push(EpollError(wa.mErrc));
                        return;
                    }

                    for(auto &[peer, event] : wa.mEvents)
                    {
                        if (event.mError & epoll_wrapper::ErrorCode::None)
                        {
                            if (event.mEventCodes & epoll_wrapper::EventCode::EpollErr)
                            {
                                mQueue.push(ConnectionError{peer.mId, event.mError});
                            }

                            if (event.mEventCodes & epoll_wrapper::EventCode::EpollHUp)
                            {
                                mQueue.push(ConnectionCloseEvent{peer.mId});
                            }

                            if (event.mEventCodes & epoll_wrapper::EventCode::EpollIn)
                            {
                                auto&& data = readFromSocket(peer.getFileDescriptor());
                                mQueue.push(ReceiveEvent{peer.mId,std::move(data)});
                            }
                        }
                    }
                }
            }

            void stop()
            {
                mIsActive = false;
            }

        private:

            bool mIsActive{false};

            Epoll<Peer> &mEpoll;
            ReceiveQueue &mQueue;
    };

    using ReceiverWorker = ReceiverWorkerImpl<PeerFd, epoll_wrapper::Epoll, WorkQueueImpl<256, PeerEvent>>;
}