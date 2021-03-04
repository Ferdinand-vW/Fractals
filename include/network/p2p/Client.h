#pragma once

#include "network/http/Tracker.h"

struct P2PStatus {
    bool m_am_choking = true;
    bool m_am_interested = false;
    bool m_peer_choking = true;
    bool m_peer_interested = false;
};

class Client {
    std::map<PeerId,P2PStatus> m_peer_status;
    
    public:

    void add_peer(PeerId p) {
        m_peer_status.insert({p,P2PStatus()})
    }


};