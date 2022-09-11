#pragma once

#include <optional>
#include <memory>
#include <sys/epoll.h>

namespace fractals::network::epoll
{
    class IEpoll
    {
        private:
            int mEpollFd{0};

            IEpoll(int epollfd);

        public:
            static std::unique_ptr<IEpoll> epoll_create(int size);
            int epoll_ctl(int op, int fd, struct epoll_event *event);
            int epoll_wait(struct epoll_event *events, int maxevents, int timeout);
            void close();
    };
}