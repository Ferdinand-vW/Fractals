#include "fractals/network/http/Announce.h"

namespace fractals::network::http
{

bool Announce::operator==(const Announce& ann) const
    {
        return announce_time == ann.announce_time
            && interval == ann.interval
            && min_interval == ann.min_interval
            && std::equal(peers.begin(), peers.end(), ann.peers.begin());
    }

}