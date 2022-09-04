#include "fractals/network/epoll/Epoll.ipp"
#include "fractals/network/p2p/Socket.h"

namespace fractals::network::epoll
{

    template<>
    class Epoll<p2p::Socket>;
        
}