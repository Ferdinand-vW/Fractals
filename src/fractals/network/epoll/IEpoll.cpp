#include "fractals/network/epoll/IEpoll.h"

#include <memory>
#include <sys/epoll.h>
#include <unistd.h>

namespace fractals::network::epoll
{
    IEpoll::IEpoll(int epollFd) : mEpollFd(epollFd) {}

    std::unique_ptr<IEpoll> IEpoll::epoll_create(int size)
    {
        int epollFd = ::epoll_create(size);
        return std::unique_ptr<IEpoll>(new IEpoll(epollFd));
    }

    int IEpoll::epoll_ctl(int op, int fd, struct epoll_event *event)
    {
        return ::epoll_ctl(mEpollFd, op, fd, event);
    }

    int IEpoll::epoll_wait(struct epoll_event *events, int maxevents, int timeout)
    {
        return ::epoll_wait(mEpollFd, events, maxevents, timeout);
    }

    void IEpoll::close()
    {
        ::close(mEpollFd);
    }
}