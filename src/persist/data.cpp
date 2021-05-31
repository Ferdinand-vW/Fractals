#include "persist/data.h"
#include "network/http/Peer.h"
#include <algorithm>
#include <functional>
#include <iterator>

void save_torrent(const Storage &st, const Torrent &t) {
    auto tm = TorrentModel { 0, t.m_name, "./metainfo/" + t.m_name + ".torrent" , "./downloads"};
    st.add_torrent(tm);
}

std::vector<std::unique_ptr<Torrent>> load_torrents(const Storage &st) {
    auto tms = st.load_torrents();

    std::vector<std::unique_ptr<Torrent>> torrs;

    std::transform(tms.begin(),tms.end(),std::back_inserter(torrs),[](auto &tm) {
        return std::make_unique<Torrent>(Torrent::read_torrent(tm.meta_info_path));
    });

    return torrs;
}

void delete_torrent(const Storage &st, const Torrent &t) {
    auto tm = st.load_torrent(t.m_name);
    if(tm.has_value()) {
        st.delete_torrent(tm.value());
    }
}

void add_piece(const Storage &st, const Torrent &t,int piece) {
    auto tm = st.load_torrent(t.m_name);
    if(tm.has_value()) {
        auto pm = PieceModel {0,tm->id,piece};
        st.add_piece(pm);
    }
}

std::vector<int> load_pieces(const Storage &st, const Torrent &t) {
    auto tm = st.load_torrent(t.m_name);
    if(tm.has_value()) {
        auto pms = st.load_pieces(tm.value());
        std::vector<int> pieces;
        std::transform(pms.begin(),pms.end(),std::back_inserter(pieces),[](PieceModel &pm) {
            return pm.piece;
        });

        return pieces;
    } else {
        return {};
    }
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