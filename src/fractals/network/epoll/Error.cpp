#include "fractals/network/epoll/Error.h"

namespace fractals::network::epoll
{
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
}