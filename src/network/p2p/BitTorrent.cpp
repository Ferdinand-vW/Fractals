#include "network/p2p/BitTorrent.h"
#include "network/http/Tracker.h"
#include "network/p2p/PeerId.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <condition_variable>
#include <cstdlib>
#include <exception>
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

PeerId BitTorrent::connect_to_a_peer() {
    auto p = choose_peer();

    while(!attempt_connect(p)) { p = choose_peer(); }

    return p;
}

PeerId BitTorrent::choose_peer() {
    int n = rand() % m_available_peers.size();
    auto it = m_available_peers.begin();
    std::advance(it,n);
    PeerId p = *it;
    m_available_peers.erase(p);
    return p;
}

bool BitTorrent::attempt_connect(PeerId p) {
    tcp::socket socket(m_io);
    auto endp = tcp::endpoint(boost::asio::ip::address::from_string(p.m_ip),p.m_port);
    cout << "[BitTorrent] connecting to peer " << p.m_ip << ":" << p.m_port << endl;
    try {
        socket.connect(endp);
        cout << "[BitTorrent] connected." << endl;
    }
    catch (std::exception &error) {
        cout << "[BitTorrent] failed to connect to peer " << p.m_ip << endl;
        return false;
    }
    
    auto shared_socket = std::make_shared<tcp::socket>(std::move(socket));
    auto shared_timer = std::make_shared<boost::asio::deadline_timer>(boost::asio::deadline_timer(m_io));

    Client c(shared_socket,shared_timer,m_torrent);
    m_client = std::make_shared<Client>(std::move(c));

    PeerListener pl(p,m_client,shared_socket);

    m_peer = std::make_shared<PeerListener>(std::move(pl));

    return true;
}

bool BitTorrent::perform_handshake() {
    std::string prot("BitTorrent protocol");
    char reserved[8] = {0,0,0,0,0,0,0,0};
    auto handshake = HandShake(prot.size(),prot,reserved,m_torrent->m_info_hash,m_client->m_client_id);
    cout << ">>> " + handshake.pprint() << endl;
    m_client->send_handshake(handshake);
    auto peer_handshake = m_peer->receive_handshake();
    cout << "<<< " + peer_handshake->pprint() << endl;

    return true;
}

void BitTorrent::run() {
    request_peers();
    cout << "[BitTorrent] num peers: " << m_available_peers.size() << endl;

    auto p = connect_to_a_peer();
    bool established_p2p = false;
    while(!established_p2p) {
        established_p2p = perform_handshake();

        if(!established_p2p) { p = connect_to_a_peer(); }
    }

    
    cout << "[BitTorrent] Using peer: " << p.m_ip << ":" << p.m_port << endl;

    m_peer->read_messages();
    m_client->wait_for_unchoke(p);

    std::thread t(boost::bind(&boost::asio::io_context::run, &m_io));
    m_io.run();
    t.join();

}