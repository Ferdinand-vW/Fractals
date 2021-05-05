#include "network/p2p/BitTorrent.h"
#include "network/http/Tracker.h"
#include "network/p2p/Connection.h"
#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/Response.h"
#include "common/logger.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/system/error_code.hpp>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <unistd.h>

BitTorrent::BitTorrent(std::shared_ptr<Torrent> torrent,boost::asio::io_context &io) 
                    : m_torrent(torrent)
                    , m_io(io)
                    , m_lg(logger::get())
                    {};

void BitTorrent::request_peers() {
    auto tr = makeTrackerRequest(m_torrent->m_mi);
    auto resp = sendTrackerRequest(tr);

    if(resp.isLeft) {
        BOOST_LOG(m_lg) << "[BitTorrent] tracker response error: " << resp.leftValue;
        if (resp.leftValue == "announcing too fast") { sleep(10); }
        request_peers();
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

    attempt_connect(p);
    while(!m_client->is_connected_to(p)) {
        p = choose_peer();
        attempt_connect(p);
    }

    return p;
}

void BitTorrent::perform_handshake(PeerId p) {
    std::string prot("BitTorrent protocol");
    char reserved[8] = {0,0,0,0,0,0,0,0};
    auto handshake = HandShake(prot.size(),prot,reserved,m_torrent->m_info_hash,m_client->m_client_id);
    BOOST_LOG(m_lg) << p.m_ip << " >>> " + handshake.pprint();
    m_client->send_handshake(p,std::move(handshake));

    m_client->await_handshake(p);
}

PeerId BitTorrent::choose_peer() {
    if(m_available_peers.size() == 0) {
        request_peers(); //request additional peers if we tried all
    }
    int n = rand() % m_available_peers.size();
    auto it = m_available_peers.begin();
    std::advance(it,n);
    PeerId p = *it;
    m_available_peers.erase(p);
    return p;
}

void BitTorrent::setup_client() {
    auto f = [this](PeerId p,PeerChange pc) {
        peer_change(p, pc);
    };
    Client c(m_torrent,m_io,f);
    m_client = std::make_shared<Client>(std::move(c));
}

void BitTorrent::attempt_connect(PeerId p) {
    BOOST_LOG(m_lg) << "[BitTorrent] connecting to peer " << p.m_ip << ":" << p.m_port;
    m_connected++;
    m_client->connect_to_peer(p);
}

void BitTorrent::peer_change(PeerId p,PeerChange pc) {
    //ensure only one thread can at a time add or remove a connection
    if (pc == PeerChange::Added) {
        perform_handshake(p);
    } else {
        m_connected--;
    }

    if(m_connected < m_max_peers && !m_client->has_all_pieces()) {
        connect_to_a_peer();
    }
    BOOST_LOG(m_lg) << "[BitTorrent] Current connections " << m_connected << "(" << m_max_peers << ")";
}

void BitTorrent::run() {
    std::thread t([this](){
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(m_io.get_executor());
        m_io.run();
    });

    setup_client();

    request_peers();
    BOOST_LOG(m_lg) << "[BitTorrent] Tracker responded with : " << m_available_peers.size() << " peers";

    //attempt to connect to peers
    connect_to_a_peer();

    t.join();
}