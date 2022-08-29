#pragma once

#include "Socket.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace fractals::network::p2p
{

    struct EpollResult
    {
        std::vector<struct epoll_event> mData;
        int32_t resultCode;
    };

    template <typename FdType>
    class Epoll
    {
        
    public:
        enum class Action { READ, WRITE };

        static std::unique_ptr<Epoll> createEpoll(int32_t timeout);

        ~Epoll();

        Epoll(const Epoll&) = delete;
        Epoll(Epoll&&) = delete;
        Epoll& operator=(const Epoll&) = delete;
        Epoll& operator=(Epoll&&) = delete;

        EpollResult wait();
        int32_t addListener(const std::unique_ptr<FdType>& fd, Action action);
        int32_t removeListener(const std::unique_ptr<FdType>& fd);
        
    private:
        static constexpr uint32_t MAXEVENTS = 32;

        std::unordered_set<int32_t> mFileDescriptors;
        int32_t mEpollFd{-1};
        struct epoll_event mEvents[MAXEVENTS];
        int32_t mTimeout{-1};
        

        Epoll(int32_t epollfd, int32_t timeout);

        int actionToEvent(Action action);
};
}