#include "Receiver.h"
#include "Event.h"

#include <sstream>



namespace fractals::network::p2p
{
    template <typename Peer, typename Epoll, typename RQ>
    ReceiverWorkerImpl<Peer, Epoll, RQ>::ReceiverWorkerImpl(Epoll& epoll, RQ &rq)
        : mEpoll(epoll), mQueue(rq) {}

    
    template <typename Peer, typename Epoll, typename RQ>
    typename epoll_wrapper::CtlAction ReceiverWorkerImpl<Peer, Epoll, RQ>::subscribe(const Peer &peer)
    {
        return mEpoll.add(peer, Epoll::EventCodes::EpollOut);
    }

    template <typename Peer, typename Epoll, typename RQ>
    typename epoll_wrapper::CtlAction ReceiverWorkerImpl<Peer, Epoll, RQ>::unsubscribe(const Peer &peer)
    {
        return mEpoll.erase(peer);
    }

    template <typename Peer, typename Epoll, typename RQ>
    void ReceiverWorkerImpl<Peer, Epoll, RQ>::run()
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

                    if (event.mEventCodes & epoll_wrapper::EventCode::EpollHUp )
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

    template <typename Peer, typename Epoll, typename RQ>
    void ReceiverWorkerImpl<Peer, Epoll, RQ>::stop()
    {
        mIsActive = false;
    }

}