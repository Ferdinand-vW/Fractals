#pragma once
/*

One Client class per Torrent
One PeerListener per Peer
One Peer per Torrent
One Client may receive messages from or send messages to multiple peers

PeerListener should constinuously listen to messages sent by associated Peer
Messages should be forwarded to the corresponding Client

(Torrent identifier has name of torrent,file names and location on disk)
Protocol should accept torrent identifier, present pieces and a list of peers
1) If 

*/
class Protocol {
    Client m_client;
    PeerListener m_peer;
    public:
        void run();
}

void start_protocol(Client c,Peer p) {
    auto ok = initiate_protocol(c, p);
    if (!ok) { return; }

    send_interest(c,p);

    rcv_unChoke(c,p);
}

bool initiate_protocol(Client c,Peer p);


show_interest(Client c,Peer p);

choke(Client c,Peer p);

unChoke(Client c, Peer p);