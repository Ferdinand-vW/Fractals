#include "Epoll.h"

#include <asm-generic/errno-base.h>
#include <cerrno>
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

    std::ostream& operator<<(std::ostream& os, const EventCode& ec)
    {
        switch(ec)
        {
        case EventCode::EpollIn:
            os << "EPOLLIN";
            break;
        case EventCode::EpollOut:
            os << "EPOLLOUT";
            break;
        case EventCode::EpollRdHUp:
            os << "EPOLLRDHUP";
            break;
        case EventCode::EpollPri:
            os << "EPOLLPRI";
            break;
        case EventCode::EpollErr:
            os << "EPOLLERR";
            break;
        case EventCode::EpollHUp:
            os << "EPOLLHUP";
            break;
        case EventCode::EpollEt:
            os << "EPOLLET";
            break;
        case EventCode::EpollOneShot:
            os << "EPOLLONESHOT";
            break;
        case EventCode::EpollWakeUp:
            os << "EPOLLWAKEUP";
            break;
        case EventCode::EpollExclusive:
            os << "EPOLLEXCLUSIVE";
            break;
        case EventCode::None:
            os << "NONE";
            break;
        }

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const ErrorCode& ec)
    {
        switch(ec)
        {
        case ErrorCode::None:
            os << "NONE";
            break;
        case ErrorCode::Unknown:
            os << "UNKNOWN";
            break;
        case ErrorCode::EbadF:
            os << "EBADF";
            break;
        case ErrorCode::Eexist:
            os << "EEXISTS";
            break;
        case ErrorCode::Einval:
            os << "EINVAL";
            break;
        case ErrorCode::Eloop:
            os << "ELOOP";
            break;
        case ErrorCode::EnoEnt:
            os << "ENOENT";
            break;
        case ErrorCode::EnoMem:
            os << "ENOMEM";
            break;
        case ErrorCode::EnoSpc:
            os << "ENOSPEC";
            break;
        case ErrorCode::Eperm:
            os << "EPERM";
            break;
        case ErrorCode::Efault:
            os << "EFAULT";
            break;
        case ErrorCode::Eintr:
            os << "EINTR";
            break;
        case ErrorCode::EmFile:
            os << "EMFILE";
            break;
        case ErrorCode::EnFile:
            os << "ENFILE";
            break;
        }

        return os;
    }

    bool Error::isSuccess()
    { 
        return mCode == ErrorCode::None;
    }

    constexpr int toEpollEvent(EventCode event)
    {
        switch (event)
        {
            case EventCode::EpollIn:
                return EPOLLIN;
            case EventCode::EpollOut:
                return EPOLLOUT;
            case EventCode::EpollRdHUp:
                return EPOLLRDHUP;
            case EventCode::EpollPri:
                return EPOLLPRI;
            case EventCode::EpollErr:
                return EPOLLERR;
            case EventCode::EpollHUp:
                return EPOLLHUP;
            case EventCode::EpollEt:
                return EPOLLET;
            case EventCode::EpollOneShot:
                return EPOLLONESHOT;
            case EventCode::EpollWakeUp:
                return EPOLLWAKEUP;
            case EventCode::EpollExclusive:
                return EPOLLEXCLUSIVE;
            case EventCode::None:
                return 0;
            }
    }

    bool EventCodes::has(EventCode ec)
    {
        return mCodes.find(ec) != mCodes.end();
    }

    EventCodes fromEpollEvent(int eventc)
    {
        std::unordered_set<EventCode> codes;

        if (eventc & EPOLLIN)        { codes.emplace(EventCode::EpollIn);        }
        if (eventc & EPOLLOUT)       { codes.emplace(EventCode::EpollOut);       }
        if (eventc & EPOLLRDHUP)     { codes.emplace(EventCode::EpollRdHUp);     }
        if (eventc & EPOLLPRI)       { codes.emplace(EventCode::EpollPri);       }
        if (eventc & EPOLLERR)       { codes.emplace(EventCode::EpollErr);       }
        if (eventc & EPOLLHUP)       { codes.emplace(EventCode::EpollHUp);       }
        if (eventc & EPOLLET)        { codes.emplace(EventCode::EpollEt);        }
        if (eventc & EPOLLONESHOT)   { codes.emplace(EventCode::EpollOneShot);   }
        if (eventc & EPOLLWAKEUP)    { codes.emplace(EventCode::EpollWakeUp);    }
        if (eventc & EPOLLEXCLUSIVE) { codes.emplace(EventCode::EpollExclusive); }

        return EventCodes{codes};
    }

    constexpr ErrorCode fromEpollError(int errc)
    {
        if (errc >= 0)
        {
            return ErrorCode::None;
        }

        int err = errno;

        if      (err & EBADF)  { return ErrorCode::EbadF;  }
        else if (err & EEXIST) { return ErrorCode::Eexist; }
        else if (err & EINVAL) { return ErrorCode::Einval; }
        else if (err & ELOOP)  { return ErrorCode::Eloop;  }
        else if (err & ENOENT) { return ErrorCode::EnoEnt; }
        else if (err & ENOMEM) { return ErrorCode::EnoMem; }
        else if (err & ENOSPC) { return ErrorCode::EnoSpc; }
        else if (err & EPERM)  { return ErrorCode::Eperm;  }
        else if (err & EFAULT) { return ErrorCode::Efault; }
        else if (err & EINTR)  { return ErrorCode::Eintr;  }
        else if (err & EMFILE) { return ErrorCode::EmFile; }
        else if (err & ENFILE) { return ErrorCode::EnFile; }
        else { return ErrorCode::Unknown; }
    }

    Error toError(int errc)
    {
        return Error{fromEpollError(errc)};
    }

        template <typename FdType>
    Epoll<FdType>::Epoll(int32_t epollfd) : mEpollFd(epollfd) {}

    template <typename FdType>
    CreateResult<FdType> Epoll<FdType>::epollCreate()
    {
        auto epollFd = epoll_create(1);

        if (epollFd != 0)
        {
            return CreateResult<FdType>{
                std::unique_ptr<Epoll<FdType>>(
                    new Epoll<FdType>(epollFd))
                , ErrorCode::None};
        }

        return {};
    }

    template <typename FdType>
    Epoll<FdType>::~Epoll()
    {
        this->close();
    }

    template <typename FdType>
    void Epoll<FdType>::close()
    {
        ::close(mEpollFd);
    }

    template <typename FdType>
    WaitResult Epoll<FdType>::wait(uint32_t timeout)
    {
        struct epoll_event events[MAXEVENTS];
        auto resultCode = epoll_wait(mEpollFd, events, MAXEVENTS, timeout);

        std::vector<Event> eventVector;
        int i = 0;

        while (i < resultCode)
        {
            Event ev{fromEpollEvent(events[i].events), fromEpollError(resultCode), std::move(events[i].data)};
            eventVector.emplace_back(std::move(ev));
            ++i;
        }

        
        return WaitResult{eventVector, ErrorCode::None};
    }

    template <typename FdType>
    CtlResult Epoll<FdType>::add(const std::unique_ptr<FdType>& socket, EventCode eventc)
    {
        auto fd = socket->getFileDescriptor();
        
        struct epoll_event event;
        event.events = toEpollEvent(eventc);
        event.data.fd = fd;
        
        std::cout << mEpollFd << std::endl;
        auto res = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &event);

        if (res)
        {
            mFileDescriptors.insert(fd);
        }

        return CtlResult{fromEpollError(res)};
    }

    template <typename FdType>
    CtlResult Epoll<FdType>::erase(const std::unique_ptr<FdType>& socket)
    {
        auto fd = socket->getFileDescriptor();

        struct epoll_event event;
        auto res = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, &event);

        if (res)
        {
            mFileDescriptors.erase(fd);
        }

        return CtlResult{fromEpollError(res)};
    }
        
}