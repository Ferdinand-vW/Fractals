#include "network/p2p/BitTorrent.h"
#include "network/http/Tracker.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/bind.hpp>
#include <condition_variable>
#include <cstdlib>
#include <memory>
#include <thread>

BitTorrent::BitTorrent(std::shared_ptr<Torrent> torrent,boost::asio::io_context &io) : m_torrent(torrent),m_io(io) {};

void BitTorrent::request_peers() {
    auto tr = makeTrackerRequest(m_torrent->m_mi);
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
    int n = rand() % m_available_peers.size();
    auto it = m_available_peers.begin();
    std::advance(it,n);
    PeerId p = *it;
    m_available_peers.erase(p);
    return p;
}

void BitTorrent::connect_to_peer(PeerId p) {
    cout << "right before" << endl;
    tcp::socket socket(m_io);
    cout << "after" << endl;
    auto endp = tcp::endpoint(boost::asio::ip::address::from_string(p.m_ip),p.m_port);
    cout << "connecting.." << endl;
    socket.connect(endp);
    
    auto shared_socket = std::make_shared<tcp::socket>(std::move(socket));
    auto shared_timer = std::make_shared<boost::asio::deadline_timer>(boost::asio::deadline_timer(m_io));
    cout << "created shared pointer to socket" << endl;

    Client c(shared_socket,shared_timer,m_torrent);
    m_client = std::make_shared<Client>(std::move(c));
    

    PeerListener pl(p,m_client,shared_socket);

    m_peer = std::make_shared<PeerListener>(std::move(pl));
}

void BitTorrent::perform_handshake() {
    std::string prot("BitTorrent protocol");
    char reserved[8] = {0,0,0,0,0,0,0,0};
    auto handshake = HandShake(prot.size(),prot,reserved,m_torrent->m_info_hash,m_client->m_client_id);
    cout << "sending handshake..." << endl;
    m_client->send_handshake(handshake);
    cout << "receiving handshake..." << endl;
    auto peer_handshake = m_peer->receive_handshake();
    cout << "received handshake" << endl;

}

void BitTorrent::run() {
    request_peers();
    cout << "num peers: " << m_available_peers.size() << endl;

    auto p = choose_peer();
    cout << "Using peer: " << p.m_ip << ":" << p.m_port << endl;

    connect_to_peer(p);
    cout << "created client and peer" << endl;
    
    perform_handshake();

    m_peer->read_messages();
    m_client->wait_for_unchoke(p);

    std::thread t(boost::bind(&boost::asio::io_context::run, &m_io));
    m_io.run();
    t.join();

}