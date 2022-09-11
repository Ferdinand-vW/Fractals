#include "fractals/network/epoll/Event.h"

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

    int fromEvent(const EventCodes& events)
    {
        int ec = 0;

        for(auto event : events.mCodes)
        {
            ec |= fromEvent(event);
        }

        return ec;
    }

    constexpr int fromEvent(EventCode event)
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

    EventCodes::EventCodes(EventCode ec) : mCodes({ec}) {}
    EventCodes::EventCodes(std::unordered_set<EventCode> ec) : mCodes(ec) {}

    EventCodes& EventCodes::operator=(const EventCodes& ec)
    {
        this->mCodes = ec.mCodes;

        return *this;
    }
    EventCodes& EventCodes::operator=(EventCodes&& ec)
    {
        this->mCodes = std::move(ec.mCodes);

        return *this;
    }

    bool EventCodes::has(EventCode ec)
    {
        return mCodes.find(ec) != mCodes.end();
    }

    bool EventCodes::operator==(const EventCodes &ecs) const
    {
        std::unordered_set<EventCode> codes;
        for (auto ec : mCodes)
        {
            if (ecs.mCodes.count(ec))
            {
                codes.insert(ec);
            }
        }

        return codes.size() == mCodes.size();
    }

    EventCodes operator| (EventCode ec1, EventCode ec2)
    {
        std::unordered_set<EventCode> codes;
        codes.insert(ec1);
        codes.insert(ec2);
        return EventCodes{codes};
    }

    EventCodes operator| (EventCodes ec1, EventCode ec2)
    {
        ec1.mCodes.insert(ec2);
        return ec1;
    }

    EventCodes operator| (EventCodes ec1, EventCodes ec2)
    {
        ec1.mCodes.insert(ec2.mCodes.begin(), ec2.mCodes.end());
        return ec1;
    }

    std::ostream& operator<<(std::ostream& os, const EventCodes& ecs)
    {
        bool first = true;
        os << "{";
        for (auto ec : ecs.mCodes)
        {
            if (!first)
            {
                os << ",";
            }
            else
            {
                first = false;
            }

            os << ec;
        }

        os << "}";

        return os;
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
}