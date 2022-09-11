#pragma once

#include <ostream>

namespace fractals::network::epoll
{
    enum class ErrorCode 
        { None, Unknown
        , EbadF , Eexist, Einval, Eloop
        , EnoEnt, EnoMem, EnoSpc, Eperm
        , Efault, Eintr , EmFile, EnFile 
        };

    std::ostream& operator<<(std::ostream&, const ErrorCode&);

    struct Error
    {
        ErrorCode mCode{ErrorCode::None};
        int mErrc{0};

        bool isSuccess();
    };

    constexpr ErrorCode fromEpollError(int errc);
    Error toError(int errc);
}