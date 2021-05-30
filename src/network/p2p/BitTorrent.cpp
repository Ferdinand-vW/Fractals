#include "network/p2p/BitTorrent.h"
#include "network/http/Tracker.h"
#include "network/p2p/Connection.h"
#include "network/p2p/Message.h"
#include "network/http/Peer.h"
#include "common/logger.h"
#include "persist/data.h"
#include <bits/types/time_t.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/thread/thread.hpp>
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

BitTorrent::BitTorrent(std::shared_ptr<Torrent> torrent
                      ,boost::asio::io_context &io
                      ,Storage st) 
                    : m_torrent(torrent)
                    , m_io(io)
                    , m_lg(logger::get())
                    , m_storage(st)
                    {};

void BitTorrent::request_peers() {
    std::unique_lock<std::recursive_mutex> lock(m_mutex,std::try_to_lock);
    //only one thread should request peers
    if(lock.owns_lock()) {
        auto mann = load_announce(m_storage, *m_torrent.get());
        if(mann.has_value()) {
            auto ann = mann.value();
            time_t curr = std::time(0);
            int iv = ann.min_interval.value_or(ann.interval);
            if (curr - ann.announce_time < iv) {

            }
        }
        auto tr = makeTrackerRequest(m_torrent->m_mi);
        auto resp = sendTrackerRequest(tr);

        if(resp.isLeft) {
            BOOST_LOG(m_lg) << "[BitTorrent] tracker response error: " << resp.leftValue;
            if (resp.leftValue == "announcing too fast") { sleep(10); }
            request_peers();
        }
        else {
            for(auto &p : resp.rightValue.peers) {
                m_available_peers.insert(p.peer_id);
            }
        }
    }
}

std::optional<PeerId> BitTorrent::connect_to_a_peer() {
    auto opt_p = choose_peer();

    if(opt_p.has_value()) {
        auto p = opt_p.value();
        attempt_connect(p);

        while(!m_client->is_connected_to(p)) {
            opt_p = choose_peer();
            if(opt_p.has_value()){
                p = opt_p.value();
                attempt_connect(p);
            } else {
                return {};
            }
        }

        return p;
    } else {
        return {};
    }
    
}

void BitTorrent::perform_handshake(PeerId p) {
    std::string prot("BitTorrent protocol");
    char reserved[8] = {0,0,0,0,0,0,0,0};
    auto handshake = HandShake(prot.size(),prot,reserved,m_torrent->m_info_hash,m_client->m_client_id);
    BOOST_LOG(m_lg) << p.m_ip << " >>> " + handshake.pprint();
    m_client->send_handshake(p,std::move(handshake));

    m_client->await_handshake(p);
}

std::optional<PeerId> BitTorrent::choose_peer() {
    if(m_available_peers.size() == 0) {
        request_peers(); //request additional peers if we tried all
    }
    if(m_available_peers.size() != 0) {
        int n = rand() % m_available_peers.size();
        auto it = m_available_peers.begin();
        std::advance(it,n);
        PeerId p = *it;
        m_available_peers.erase(p);
        return p;
    } else {
        return {};
    }
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
    BOOST_LOG(m_lg) << "peer change";
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
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(m_io.get_executor());

    boost::thread_group threads;

    for( unsigned int i = 0; i < 4; i++ ) {
        threads.create_thread(
            [&]() { m_io.run(); }
        );
}

    setup_client();

    request_peers();
    BOOST_LOG(m_lg) << "[BitTorrent] Tracker responded with : " << m_available_peers.size() << " peers";

    //attempt to connect to peers
    connect_to_a_peer();

    threads.join_all();
}