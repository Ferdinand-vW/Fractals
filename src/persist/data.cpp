#include "persist/data.h"
#include "common/maybe.h"
#include "network/http/Peer.h"
#include <algorithm>
#include <functional>
#include <iterator>

void save_torrent(const Storage &st, const Torrent &t) {
    auto tm = TorrentModel { 0, t.m_name, "./metainfo/" + t.m_name + ".torrent" , "./downloads"};
    st.add_torrent(tm);
}

std::vector<std::shared_ptr<Torrent>> load_torrents(const Storage &st) {
    auto tms = st.load_torrents();

    std::vector<std::shared_ptr<Torrent>> torrs;

    for(auto &tm : tms) {
        auto t = Torrent::read_torrent(tm.meta_info_path);
        if(!t.isLeft) {
            torrs.push_back(std::make_shared<Torrent>(t.rightValue));
        }
    }

    return torrs;
}

bool has_torrent(const Storage &st, const Torrent &t) {
    return st.load_torrent(t.m_name).has_value();
}

void delete_torrent(const Storage &st, const Torrent &t) {
    auto tm = st.load_torrent(t.m_name);
    if(tm.has_value()) {
        st.delete_torrent(tm.value());
    }
}

void save_piece(const Storage &st, const Torrent &t,int piece) {
    auto tm = st.load_torrent(t.m_name);
    if(tm.has_value()) {
        auto pm = PieceModel {0,tm->id,piece};
        st.add_piece(pm);
    }
}

std::set<int> load_pieces(const Storage &st, const Torrent &t) {
    auto tm = st.load_torrent(t.m_name);
    std::set<int> pieces;
    if(tm.has_value()) {
        auto pms = st.load_pieces(tm.value());
        
        for(auto &pm : pms) {
            pieces.insert(pm.piece);
        }
    }

    return pieces;
}

void save_announce(const Storage &st, const Torrent &t, const Announce &ann) {
    auto tm = st.load_torrent(t.m_name);
    if(tm.has_value()) {
        auto peers = ann.peers;
        std::vector<AnnounceModel> ams;
        std::transform(peers.begin(),peers.end(),std::back_inserter(ams),[&tm,&ann](PeerId &p) {
            return AnnounceModel{0,tm->id,p.m_ip,p.m_port,ann.announce_time, ann.interval, ann.min_interval };
        });
        
        std::for_each(ams.begin(),ams.end(),[&st](auto &am) { st.save_announce(am); });
    }
}

void delete_announces(const Storage &st, const Torrent &t) {
    auto tm = st.load_torrent(t.m_name);
    if(tm.has_value()) {
        st.delete_announces(tm.value());
    }
}

std::optional<Announce> load_announce(const Storage &st, const Torrent &t) {
    auto tm_opt = st.load_torrent(t.m_name);
    if(!tm_opt.has_value()) { return {}; }
    auto tm = tm_opt.value();

    auto ams = st.load_announce(tm);

    //if ams is empty then return nothing
    if(ams.begin() == ams.end()) {
        return {};
    }

    auto am = ams.front();
    std::vector<PeerId> peers;
    std::transform(ams.begin(),ams.end(),std::back_insert_iterator(peers),[](auto &am){
        return PeerId { am.peer_ip,am.peer_port};
    });

    return Announce{am.announce_time,am.interval,am.min_interval,peers};

}