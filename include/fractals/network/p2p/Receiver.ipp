#include "Receiver.h"
#include "Event.h"

#include <sstream>



namespace fractals::network::p2p
{
    template <typename Peer, typename Epoll, template <typename> typename RQ>
    ReceiverWorkerImpl<Peer, Epoll, RQ>::ReceiverWorkerImpl(Epoll& epoll, RQ<Peer> &rq)
        : mEpoll(epoll), mQueue(rq) {}

    
    template <typename Peer, typename Epoll, template <typename> typename RQ>
    typename Epoll::Error ReceiverWorkerImpl<Peer, Epoll, RQ>::subscribe(const std::unique_ptr<Peer> &peer)
    {
        return mEpoll.add(peer, Epoll::EventCodes::EpollIn);
    }

    template <typename Peer, typename Epoll, template <typename> typename RQ>
    typename Epoll::Error ReceiverWorkerImpl<Peer, Epoll, RQ>::unsubscribe(const std::unique_ptr<Peer> &peer)
    {
        return mEpoll.erase(peer);
    }

    template <typename Peer, typename Epoll, template <typename> typename RQ>
    void ReceiverWorkerImpl<Peer, Epoll, RQ>::run()
    {
        mIsActive = true;

        while(mIsActive)
        {
            auto wa = mEpoll.wait();

            if (wa.hasError())
            {
                std::stringstream ss;
                ss << wa.mErrorCode;
                mQueue.push(EpollError{{}, ss.str()});
            }

            for (auto &event : wa.mEvents)
            {
                
                if (event.hasError())
                {
                    std::stringstream ss;
                    ss << event.mErrorCode;
                    mQueue.push(ReceiveError{{}, ss.str(), })
                }
                else
                {
                    mQueue.push(PeerReceiveData(event.mData));
                }
            }
        }
    }

    template <typename Peer, typename Epoll, template <typename> typename RQ>
    void ReceiverWorkerImpl<Peer, Epoll, RQ>::stop()
    {
        mIsActive = false;
    }

}