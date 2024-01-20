#include <fractals/network/p2p/EpollService.ipp>
#include <fractals/network/p2p/BufferedQueueManager.h>
#include <fractals/network/p2p/EpollMsgQueue.h>

namespace fractals::network::p2p
{
    template class EpollServiceImpl<PeerFd, epoll_wrapper::Epoll<PeerFd>, BufferedQueueManager, EpollMsgQueue>;
}