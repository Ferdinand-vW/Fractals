#pragma once

#include "Error.h"

#include <ostream>
#include <vector>
#include <sys/epoll.h>
#include <unordered_set>

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

    struct EventCodes
    {
        EventCodes() = default;
        EventCodes(EventCode ec);
        EventCodes(std::unordered_set<EventCode> ec);
        EventCodes(const EventCodes &) = default;
        EventCodes(EventCodes &&) = default;

        EventCodes& operator=(const EventCodes&);
        EventCodes& operator=(EventCodes&&);

        std::unordered_set<EventCode> mCodes;

        bool has(EventCode ec);

        std::unordered_set<EventCode>* operator->()
        {
            return &mCodes;
        }

        bool operator==(const EventCodes &ec) const;
    };

    
    EventCodes operator| (EventCode ec1, EventCode ec2);
    EventCodes operator| (const EventCodes& ec1, EventCode ec2);
    EventCodes operator| (const EventCodes& ec1, const EventCodes& ec2);
    std::ostream& operator<<(std::ostream&, const EventCodes&);

    struct Event
    {
        EventCodes mCodes;
        ErrorCode mError{ErrorCode::None};
        epoll_data_t mData;
    };

    int fromEvent(const EventCodes& events);
    constexpr int fromEvent(EventCode event);
    EventCodes fromEpollEvent(int eventc);
}