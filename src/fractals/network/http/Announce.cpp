#include "fractals/network/http/Announce.h"

namespace fractals::network::http
{

bool Announce::operator==(const Announce& ann) const
    {
        return announceTime == ann.announceTime
            && interval == ann.interval
            && minInterval == ann.minInterval
            && std::equal(peers.begin(), peers.end(), ann.peers.begin());
    }

}