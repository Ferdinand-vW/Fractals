#include "Epoll.h"

#include <cstdint>
#include <exception>
#include <optional>
#include <stdexcept>
#include <sys/epoll.h>

namespace fractals::network::p2p
{

    template <typename FdType>
    Epoll<FdType>::Epoll(int32_t epollfd, int32_t timeout) 
                : mEpollFd(epollfd)
                , mTimeout(timeout) {}

    template <typename FdType>
    std::unique_ptr<Epoll<FdType>> Epoll<FdType>::createEpoll(int32_t timeout)
    {
        auto epollFd = epoll_create1(timeout);

        if (epollFd != 0)
        {
            return std::unique_ptr<Epoll>(
                new Epoll(epollFd, timeout));
        }

        return {};
    }

    template <typename FdType>
    Epoll<FdType>::~Epoll()
    {
        close(mEpollFd);
    }

    template <typename FdType>
    int Epoll<FdType>::actionToEvent(Epoll::Action action)
    {
        switch (action)
        {
            case Action::READ:
                return EPOLLIN;
                break;
            case Action::WRITE:
                return EPOLLOUT;
                break;
        }
    }

    template <typename FdType>
    EpollResult Epoll<FdType>::wait()
    {
        struct epoll_event events[MAXEVENTS];
        auto resultCode = epoll_wait(mEpollFd, events, MAXEVENTS, mTimeout);

        std::vector<struct epoll_event> eventVector(std::begin(events), std::end(events));
        
        return EpollResult{eventVector, resultCode};
    }

    template <typename FdType>
    int32_t Epoll<FdType>::addListener(const std::unique_ptr<FdType>& socket, Epoll::Action action)
    {
        auto fd = socket->getFileDescriptor();
        
        struct epoll_event event;
        event.events = actionToEvent(action);
        event.data.fd = fd;
        
        auto res = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &event);

        if (res)
        {
            mFileDescriptors.insert(fd);
        }

        return res;
    }

    template <typename FdType>
    int32_t Epoll<FdType>::removeListener(const std::unique_ptr<FdType>& socket)
    {
        auto fd = socket->getFileDescriptor();

        struct epoll_event event;
        auto res = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, &event);

        if (res)
        {
            mFileDescriptors.erase(fd);
        }

        return res;
    }
        
}