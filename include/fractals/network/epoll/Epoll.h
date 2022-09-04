#pragma once

#include "fractals/network/p2p/Socket.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace fractals::network::epoll
{
    enum class EventCode 
        { None
        , EpollIn , EpollOut    , EpollRdHUp
        , EpollPri, EpollErr    , EpollHUp
        , EpollEt , EpollOneShot, EpollWakeUp
        , EpollExclusive
        };

    std::ostream& operator<<(std::ostream&, const EventCode&);

    enum class ErrorCode 
        { None, Unknown
        , EbadF , Eexist, Einval, Eloop
        , EnoEnt, EnoMem, EnoSpc, Eperm
        , Efault, Eintr , EmFile, EnFile 
        };

    std::ostream& operator<<(std::ostream&, const ErrorCode&);

    struct EventCodes
    {
        std::unordered_set<EventCode> mCodes;

        bool has(EventCode ec);
    };

    struct Event
    {
        EventCodes mCodes;
        ErrorCode mError{ErrorCode::None};
        epoll_data_t mData;
    };

    struct Error
    {
        ErrorCode mCode{ErrorCode::None};
        int mErrc{0};

        bool isSuccess();
    };

    
    constexpr int toEpollEvent(EventCode event);
    EventCodes fromEpollEvent(int eventc);
    constexpr ErrorCode fromEpollError(int errc);
    Error toError(int errc);

    template<typename FdType>
    class Epoll;

    template <typename FdType>
    struct CreateResult
    {
        std::unique_ptr<Epoll<FdType>> mEpoll; 
        ErrorCode mErrc;

        Epoll<FdType>* operator->()
        {
            return mEpoll.get();
        }
    };

    struct CtlResult
    {
        ErrorCode mErrc;

    };

    struct WaitResult
    {
        std::vector<Event> mEvents;
        ErrorCode mErrc;

        std::vector<Event>* operator->()
        {
            return &mEvents;
        }
    };

    template <typename FdType>
    class Epoll
    {
        
    public:
        static CreateResult<FdType> epollCreate();

        ~Epoll();

        Epoll(const Epoll&) = delete;
        Epoll(Epoll&&) = delete;
        Epoll& operator=(const Epoll&) = delete;
        Epoll& operator=(Epoll&&) = delete;

        WaitResult wait(uint32_t timeout = -1);
        CtlResult add(const std::unique_ptr<FdType>& fd, EventCode event);
        CtlResult add(const std::unique_ptr<FdType>& fd, const std::vector<EventCode>& events);
        CtlResult mod(const std::unique_ptr<FdType>& fd, EventCode event);
        CtlResult mod(const std::unique_ptr<FdType>& fd, const std::vector<EventCode>& event);
        CtlResult erase(const std::unique_ptr<FdType>& fd);
        void close();
        
    private:
        static constexpr uint32_t MAXEVENTS = 32;

        int32_t mEpollFd{-1};
        int32_t mTimeout{-1};
        std::unordered_set<int32_t> mFileDescriptors;
        struct epoll_event mEvents[MAXEVENTS];

        Epoll(int32_t epollfd);
};
}