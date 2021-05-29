#include "persist/data.h"

void add_torrent(const Storage &st, const Torrent &t) {
    auto tm = TorrentModel { 0, t.m_name, "./metainfo/" + t.m_name + ".torrent" , "./downloads"};
    st.add_torrent(tm);
}

std::vector<std::unique_ptr<Torrent>> load_torrents(const Storage &st) {
    auto tms = st.load_torrents();

    std::vector<std::unique_ptr<Torrent>> torrs;

    std::transform(tms.begin(),tms.end(),torrs.end(),[](auto &tm) {
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
        std::transform(pms.begin(),pms.end(),pieces.end(),[](auto &pm) {
            return pm.id;
        });

        return pieces;
    } else {
        return {};
    }
}

void add_announce(const Storage &st, const Torrent &t, const Announce &ann) {

}

std::optional<Announce> load_announce(const Storage &st, const Torrent &t) {

}