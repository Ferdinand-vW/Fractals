#include <cstdlib>
#include <memory>
#include <mutex>
#include <unistd.h>

#include "fractals/common/logger.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/http/Announce.h"
#include "fractals/network/http/Tracker.h"
#include "fractals/network/p2p/BitTorrent.h"
#include "fractals/network/p2p/Message.h"
#include "fractals/network/p2p/Client.h"
#include "fractals/persist/data.h"
#include "fractals/torrent/Torrent.h"

using namespace fractals::torrent;
using namespace fractals::persist;
using namespace fractals::network::http;

namespace fractals::network::p2p {

    BitTorrent::BitTorrent(std::shared_ptr<Torrent> torrent
                        ,boost::asio::io_context &io
                        ,Storage &st) 
                        : m_torrent(torrent)
                        , m_io(io)
                        , m_lg(common::logger::get())
                        , m_storage(st)
                        , mAnnService(st,torrent->getMeta())
                        {};

    int BitTorrent::connected_peers() {
        if(m_client == nullptr) {
            return 0;
        } else {
            return m_client->num_connections();
        }
    }
    int BitTorrent::available_peers() {
        return m_available_peers.size();
    }
    int BitTorrent::known_leecher_count() const {
        return m_leecher_count;
    }

    void BitTorrent::request_peers() {
        auto annOpt = mAnnService.announce();

        if(annOpt.has_value()) {
            for(auto &p : annOpt.value().peers) {
                m_available_peers.insert(p);
            }

            mAnnService.saveAnnounce(annOpt.value());
            BOOST_LOG(m_lg) << "[BitTorrent] Made new announce";
        } else {
            BOOST_LOG(m_lg) << "[BitTorrent] Announce service unavailable";
        }

    }

    void BitTorrent::connect_to_a_peer() {

        while(m_client->is_enabled()) {
            auto peerOpt = choose_peer();
            if(peerOpt.has_value()) {
                attempt_connect(peerOpt.value());
                return;
            }
        }
    }

    void BitTorrent::perform_handshake(PeerId p) {
        std::string prot("BitTorrent protocol");
        //these identify implemented BT extensions
        //currently none are implemented
        char reserved[8] = {0,0,0,0,0,0,0,0};
        auto handshake = HandShake(prot.size(),prot,reserved,m_torrent->getMeta().getInfoHash(),m_client->m_client_id);
        BOOST_LOG(m_lg) << p.m_ip << " >>> " + handshake.pprint();
        m_client->send_handshake(p,std::move(handshake));

        m_client->await_handshake(p);
    }

    std::optional<PeerId> BitTorrent::choose_peer() {
        std::unique_lock<std::mutex> lck(mMutex);
        if(m_available_peers.size() == 0) {
            request_peers(); //request additional peers if we tried all
        }
        if(m_available_peers.size() != 0) {
            int n = rand() % m_available_peers.size();
            auto it = m_available_peers.begin();
            //moves @n times on the iterator
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

        auto pieces = load_pieces(m_storage, m_torrent->getMeta());
        m_torrent->add_piece(pieces);
        m_client = std::shared_ptr<Client>(new Client(m_max_peers,m_torrent,m_io,m_storage,f));
    }

    void BitTorrent::attempt_connect(PeerId p) {
        BOOST_LOG(m_lg) << "[BitTorrent] connecting to peer " << p.m_ip << ":" << p.m_port;
        m_client->connect_to_peer(p);
    }

    void BitTorrent::peer_change(PeerId p,PeerChange pc) {
        //ensure only one thread can at a time add or remove a connection
        if (pc == PeerChange::Added) {
            perform_handshake(p);
        }

        if(!m_client->has_all_pieces() && m_client->is_enabled()) {
            connect_to_a_peer();
        }
    }

    void BitTorrent::run() {
        BOOST_LOG(m_lg) << "[BitTorrent] run";
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

}