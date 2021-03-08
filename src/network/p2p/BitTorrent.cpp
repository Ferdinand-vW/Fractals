#include "network/p2p/BitTorrent.h"

BitTorrent::BitTorrent(Torrent &t) : m_torrent(t) {};

void setup() {

}

void perform_handshake() {

}


ConnectionEnded read_messages(std::shared_ptr<Client> client,std::shared_ptr<PeerListener> pl) {
    PeerId p = pl->get_peerId();
    while (true) {
        auto m = pl->wait_message();
        auto mt = m->get_messageType();
        if(mt.has_value()) {
            switch(mt.value()) {
                case MessageType::MT_Choke:
                    client->received_choke(p);

                case MessageType::MT_UnChoke:
                    client->received_unchoke(p);

                case MessageType::MT_Interested:;
                    client->received_interested(p);

                case MessageType::MT_NotInterested:
                    client->received_not_interested(p);

                case MessageType::MT_Have: {
                    auto h = static_cast<Have*>(m.get());
                    client->received_have(p, h->m_piece_index); 
                };

                case MessageType::MT_Bitfield: {
                    auto bf = static_cast<Bitfield*>(m.get());
                    client->received_bitfield(p, *bf); 
                };

                case MessageType::MT_Request: {
                    auto r = static_cast<Request*>(m.get());
                    client->received_request(p, *r);
                };

                case MessageType::MT_Piece: {
                    auto pc = static_cast<Piece*>(m.get());
                    client->received_piece(p, *pc);
                };
                case MessageType::MT_Cancel:;
                case MessageType::MT_Port:;
            }
        } else if (m->get_length() > 0) { 
            client->received_garbage(p);
            return ConnectionEnded::ReceivedGarbage; 
        } // else it's a KeepAlive message which we can ignore
    }
}


void BitTorrent::run() {
    request_peers();

    auto p = choose_peer();

    connect_to_peer(p);
    
    perform_handshake();

    startThread(read_messages(m_client,m_peer));

}