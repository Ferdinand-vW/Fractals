#include "network/p2p/BitTorrent.h"
#include "network/http/Tracker.h"
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <memory>
#include <thread>

BitTorrent::BitTorrent(Torrent &t) : m_torrent(t) {};

void BitTorrent::request_peers() {
    auto tr = makeTrackerRequest(m_torrent.m_mi);
    auto resp = sendTrackerRequest(tr);

    if(resp.isLeft) {
        cout << "Failed to receive tracker response with error " << resp.leftValue << endl;
    }
    else {
        for(auto &p : resp.rightValue.peers) {
            PeerId peer = PeerId { p.ip,p.port };
            m_available_peers.insert(peer);
        }
    }
}


PeerId BitTorrent::choose_peer() {
    auto it = m_available_peers.begin();
    PeerId p = *it;
    m_available_peers.erase(p);
    return p;
}

void BitTorrent::connect_to_peer(PeerId p) {
    boost::asio::io_service io_service;
    tcp::socket socket(io_service);
    auto endp = tcp::endpoint(boost::asio::ip::address::from_string(p.m_ip),p.m_port);
    cout << "connecting.." << endl;
    socket.connect(endp);
    

    auto shared_socket = std::make_shared<tcp::socket>(std::move(socket));
    cout << "created shared pointer to socket" << endl;

    Client c(shared_socket,m_torrent);
    PeerListener pl(p,shared_socket);

    m_client = std::make_shared<Client>(c);
    m_peer = std::make_shared<PeerListener>(pl);
}

void BitTorrent::perform_handshake() {
    std::string prot("BitTorrent protocol");
    char reserved[8] = {0,0,0,0,0,0,0,0};
    auto handshake = HandShake(prot.size(),prot,reserved,m_torrent.m_info_hash,m_client->m_client_id);
    cout << "sending handshake..." << endl;
    m_client->send_handshake(handshake);
    cout << "receiving handshake..." << endl;
    auto peer_handshake = m_peer->receive_handshake();
    cout << "received handshake" << endl;

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
                    break;

                case MessageType::MT_UnChoke:
                    client->received_unchoke(p);
                    break;

                case MessageType::MT_Interested:;
                    client->received_interested(p);
                    break;

                case MessageType::MT_NotInterested:
                    client->received_not_interested(p);
                    break;

                case MessageType::MT_Have: {
                    auto h = static_cast<Have*>(m.get());
                    client->received_have(p, h->m_piece_index);
                    break;
                };

                case MessageType::MT_Bitfield: {
                    auto bf = static_cast<Bitfield*>(m.get());
                    client->received_bitfield(p, *bf);
                    break; 
                };

                case MessageType::MT_Request: {
                    auto r = static_cast<Request*>(m.get());
                    client->received_request(p, *r);
                    break;
                };

                case MessageType::MT_Piece: {
                    auto pc = static_cast<Piece*>(m.get());
                    client->received_piece(p, *pc);
                    break;
                };
                case MessageType::MT_Cancel: break;
                case MessageType::MT_Port: break;
            }
        } else if (m->get_length() > 0) { 
            client->received_garbage(p);
            return ConnectionEnded::ReceivedGarbage; 
        } // else it's a KeepAlive message which we can ignore
    }
}

void BitTorrent::run() {
    request_peers();
    cout << "num peers: " << m_available_peers.size() << endl;

    auto p = choose_peer();
    cout << "Using peer: " << p.m_ip << ":" << p.m_port << endl;

    connect_to_peer(p);
    cout << "created client and peer" << endl;
    
    perform_handshake();

    std::thread peer_thread(read_messages,m_client,m_peer);

    peer_thread.join();
    // startThread(read_messages(m_client,m_peer));

}