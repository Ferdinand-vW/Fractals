#pragma once

#include "torrent/Torrent.h"
#include "network/p2p/Client.h"
#include "network/p2p/PeerListener.h"
#include "network/p2p/PeerId.h"
/*

One Client class per Torrent
One PeerListener per Peer
Multiple Peer per Torrent
Peer has only one Torrent

One Client may receive messages from or send messages to multiple peers

PeerListener should constinuously listen to messages sent by associated Peer
Messages should be forwarded to the corresponding Client

(Torrent identifier has name of torrent,file names and location on disk)
Protocol should accept torrent identifier, present pieces and a list of peers
1) If 

*/
class BitTorrent {
    std::shared_ptr<Client> m_client;
    std::shared_ptr<PeerListener> m_peer;
    std::set<PeerId> m_available_peers;
    Torrent m_torrent;
    public:
        BitTorrent (Torrent &t);
        void run();

    private:
        void request_peers();
        PeerId choose_peer();
        void connect_to_peer(PeerId p);
        void perform_handshake();
};
