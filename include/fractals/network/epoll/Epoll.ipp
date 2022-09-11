#include "Epoll.h"
#include "Event.h"
#include "Error.h"
#include "fractals/common/utils.h"

#include <algorithm>
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iostream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>

namespace fractals::network::epoll
{

    template <typename EpollType>
    EpollImpl<EpollType>::EpollImpl(std::unique_ptr<EpollType>&& epoll) : mEpoll(std::move(epoll)) {}

    template <typename EpollType>
    CreateEpollImpl<EpollType> EpollImpl<EpollType>::epollCreate()
    {
        auto epollFd = EpollType::epoll_create(1);

        if (epollFd)
        {
            return CreateEpollImpl<EpollType>{
                std::unique_ptr<EpollImpl<EpollType>>(
                    new EpollImpl<EpollType>(std::move(epollFd)))
                , ErrorCode::None};
        }

        return {};
    }

    template <typename EpollType>
    EpollImpl<EpollType>::~EpollImpl()
    {
        this->close();
    }

    template <typename EpollType>
    void EpollImpl<EpollType>::close()
    {
        mEpoll->close();
    }

    template <typename EpollType>
    WaitAction EpollImpl<EpollType>::wait(uint32_t timeout)
    {
        struct epoll_event events[MAXEVENTS];
        auto resultCode = mEpoll->epoll_wait(events, MAXEVENTS, timeout);

        std::vector<Event> eventVector;
        int i = 0;

        while (i < resultCode)
        {
            Event ev{fromEpollEvent(events[i].events), fromEpollError(resultCode), std::move(events[i].data)};
            eventVector.emplace_back(std::move(ev));
            ++i;
        }

        
        return WaitAction{eventVector, ErrorCode::None};
    }

    template <typename EpollType>
    template <typename FdType>
    CtlAction EpollImpl<EpollType>::add(const std::unique_ptr<FdType>& fd_ptr, EventCodes eventc)
    {
        auto fd = fd_ptr->getFileDescriptor();
        
        struct epoll_event event;
        event.events = fromEvent(eventc);
        event.data.fd = fd;
        
        auto res = mEpoll->epoll_ctl(EPOLL_CTL_ADD, fd, &event);

        if (res == 0)
        {
            mFds.insert({fd, eventc});
        }

        return CtlAction{fromEpollError(res)};
    }

    template <typename EpollType>
    template <typename FdType>
    CtlAction EpollImpl<EpollType>::mod(const std::unique_ptr<FdType>& fd_ptr, EventCodes eventc)
    {
        auto fd = fd_ptr->getFileDescriptor();

        if (mFds.find(fd) == mFds.end())
        {
            return CtlAction{ErrorCode::EnoEnt};
        }

        struct epoll_event event;
        event.events = fromEvent(eventc);
        event.data.fd = fd;
        
        auto res = mEpoll->epoll_ctl(EPOLL_CTL_MOD, fd, &event);

        if (res == 0)
        {
            mFds[fd] = eventc;
        }

        return CtlAction{fromEpollError(res)};
    }

    template <typename EpollType>
    template <typename FdType>
    CtlAction EpollImpl<EpollType>::erase(const std::unique_ptr<FdType>& fd_ptr)
    {
        auto fd = fd_ptr->getFileDescriptor();

        struct epoll_event event;
        auto res = mEpoll->epoll_ctl(EPOLL_CTL_DEL, fd, &event);

        if (res == 0)
        {
            mFds.erase(fd);
        }

        return CtlAction{fromEpollError(res)};
    }
        
    template <typename EpollType>
    const std::unique_ptr<EpollType>& EpollImpl<EpollType>::getUnderlying() const
    {
        return mEpoll;
    }

    template <typename EpollType>
    template <typename FdType>
    const EventCodes EpollImpl<EpollType>::getEvents(const std::unique_ptr<FdType> &fd_ptr) const
    {
        auto it  = mFds.find(fd_ptr->getFileDescriptor());

        if (it != mFds.end())
        {
            return it->second;
        }

        return EventCodes({});
    }
}