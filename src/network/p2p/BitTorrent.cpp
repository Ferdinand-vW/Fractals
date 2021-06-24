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
                      ,Storage &st) 
                    : m_torrent(torrent)
                    , m_io(io)
                    , m_lg(logger::get())
                    , m_storage(st)
                    {};


std::optional<Announce> get_recent_announce(Storage &st,const Torrent &t) {
    auto mann = load_announce(st, t);
    if(mann.has_value()) {
        auto ann = mann.value();
        time_t curr = std::time(0);
        int iv = ann.min_interval.value_or(ann.interval);
        //if a recent announce exists then return those peers
        //we should not announce more often than @min_interval@
        if (curr - ann.announce_time < iv) {
            BOOST_LOG(logger::get()) << "[BitTorrent] recent announce exists";
            return ann;
        } else {
            //return nothing since the announce stored in database is not recent enough
            return {};
        }
    }

    return {};

}

std::optional<Announce> make_announce(const Torrent &t) {
    auto tr = makeTrackerRequest(t.m_mi);
    time_t curr = std::time(0);
    auto resp = sendTrackerRequest(tr);

    if(resp.isLeft) {
        BOOST_LOG(logger::get()) << "[BitTorrent] tracker response error: " << resp.leftValue;
        if (resp.leftValue == "announcing too fast") { sleep(10); }
        return {};
    }
    else {
        BOOST_LOG(logger::get()) << "[BitTorrent] tracker response: " << resp.rightValue;
        return toAnnounce(curr,resp.rightValue);
    }
}

void BitTorrent::request_peers() {
    std::unique_lock<std::recursive_mutex> lock(m_mutex,std::try_to_lock);
    //only one thread should request peers
    if(lock.owns_lock()) {
        auto &torr = *m_torrent.get();
        auto mann = get_recent_announce(m_storage,torr);

        std::vector<PeerId> new_peers;
        if(mann.has_value()) { //recent announce exists so use already known peers
            BOOST_LOG(m_lg) << "[BitTorrent] loaded recent announce";
            new_peers = mann->peers;
        } else {//make a new announce
            auto next_ann = make_announce(torr);
            if(next_ann.has_value()) { //on success add received peers
                BOOST_LOG(m_lg) << "[BitTorrent] made new recent announce";
                new_peers = next_ann->peers;
                delete_announces(m_storage,torr);
                save_announce(m_storage, torr, next_ann.value()); //saves announce in db
            } else { //indicates a failure in making the announce
                     //we retry after short sleep
                     //the 'current' invocation of request_peers will not add new peers
                request_peers(); //this should probably be done async to avoid stack overflow
            }
        }

        //add new peers to set of known peers
        for(auto &p : new_peers) {
            m_available_peers.insert(p);
        }
    }
}

std::optional<PeerId> BitTorrent::connect_to_a_peer() {
    auto opt_p = choose_peer();

    if(opt_p.has_value()) {
        auto p = opt_p.value();
        attempt_connect(p);

        while(!m_client->is_connected_to(p) && m_client->is_enabled()) {
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

    auto pieces = load_pieces(m_storage, *m_torrent.get());
    m_torrent->add_piece(pieces);
    m_client = std::shared_ptr<Client>(new Client(m_max_peers,m_torrent,m_io,m_storage,f));
}

void BitTorrent::attempt_connect(PeerId p) {
    BOOST_LOG(m_lg) << "[BitTorrent] connecting to peer " << p.m_ip << ":" << p.m_port;
    m_client->connect_to_peer(p);
}

void BitTorrent::peer_change(PeerId p,PeerChange pc) {
    BOOST_LOG(m_lg) << "peer change";
    //ensure only one thread can at a time add or remove a connection
    if (pc == PeerChange::Added) {
        perform_handshake(p);
    }

    if(!m_client->has_all_pieces() && m_client->is_enabled()) {
        BOOST_LOG(m_lg) << m_client->is_enabled() << " is enabled";
        connect_to_a_peer();
    }
    BOOST_LOG(m_lg) << "[BitTorrent] Current connections " << m_connected << "(" << m_max_peers << ")";
}

void BitTorrent::run() {
    BOOST_LOG(m_lg) << "[BitTorrent] run";
    m_max_peers = 7;
    setup_client();

    request_peers();
    BOOST_LOG(m_lg) << "[BitTorrent] Tracker responded with : " << m_available_peers.size() << " peers";

    //attempt to connect to peers
    connect_to_a_peer();
}

void BitTorrent::stop() {
    BOOST_LOG(m_lg) << "[BitTorrent] stop";
    m_client->close_connections();
}