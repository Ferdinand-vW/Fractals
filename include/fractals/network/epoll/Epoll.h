#pragma once

#include "fractals/network/p2p/Socket.h"
#include "Error.h"
#include "Event.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fractals::network::epoll
{
    template<typename EpollType>
    class EpollImpl;

    template <typename EpollType>
    struct CreateEpollImpl
    {
        std::unique_ptr<EpollImpl<EpollType>> mEpoll; 
        ErrorCode mErrc;

        EpollImpl<EpollType>* operator->()
        {
            return mEpoll.get();
        }
    };

    struct CtlAction
    {
        ErrorCode mErrc;
    };

    struct WaitAction
    {
        std::vector<Event> mEvents;
        ErrorCode mErrc;

        std::vector<Event>* operator->()
        {
            return &mEvents;
        }
    };

    template <typename EpollType>
    class EpollImpl
    {
        
    public:
        static CreateEpollImpl<EpollType> epollCreate();

        ~EpollImpl();

        EpollImpl(const EpollImpl&) = delete;
        EpollImpl(EpollImpl&&) = delete;
        EpollImpl& operator=(const EpollImpl&) = delete;
        EpollImpl& operator=(EpollImpl&&) = delete;

        WaitAction wait(uint32_t timeout = -1);

        template <typename FdType>
        CtlAction add(const std::unique_ptr<FdType>& fd, EventCodes event);
        template <typename FdType>
        CtlAction mod(const std::unique_ptr<FdType>& fd, EventCodes event);
        template <typename FdType>
        CtlAction erase(const std::unique_ptr<FdType>& fd);
        void close();

        const std::unique_ptr<EpollType>& getUnderlying() const;
        template <typename FdType> 
        const EventCodes getEvents(const std::unique_ptr<FdType> &fd) const;
        
    private:
        static constexpr uint32_t MAXEVENTS = 32;

        int32_t mTimeout{-1};
        std::unordered_map<int32_t, EventCodes> mFds;
        struct epoll_event mEvents[MAXEVENTS];

        std::unique_ptr<EpollType> mEpoll;

        EpollImpl(std::unique_ptr<EpollType>&& epollfd);
};
}