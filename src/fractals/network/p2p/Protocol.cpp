#include "fractals/network/p2p/Protocol.ipp"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/network/p2p/Protocol.h"

namespace fractals::network::p2p
{
    template class Protocol<PeerService>;
}