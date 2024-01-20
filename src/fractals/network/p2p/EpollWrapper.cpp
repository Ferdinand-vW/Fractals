#include <fractals/network/p2p/PeerFd.h>
#include <epoll_wrapper/Epoll.h>
#include <epoll_wrapper/EpollImpl.ipp>
#include <epoll_wrapper/Light.h>

namespace epoll_wrapper
{
    template class CreateAction<Epoll<fractals::network::p2p::PeerFd>>;
    template class WaitAction<fractals::network::p2p::PeerFd>;
    template class EpollImpl<Light, fractals::network::p2p::PeerFd>;
}