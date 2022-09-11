#include "fractals/network/epoll/Epoll.ipp"
#include "fractals/network/epoll/IEpoll.h"
#include "fractals/network/p2p/Socket.h"

namespace fractals::network::epoll
{

    template<>
    class EpollImpl<IEpoll>;
        
}