#include <algorithm>
#include <filesystem>
#include <iterator>

#include <neither/either.hpp>

#include "fractals/network/http/Peer.h"
#include "fractals/network/http/Announce.h"
#include "fractals/persist/announce_model.h"
#include "fractals/persist/data.h"
#include "fractals/persist/piece_model.h"
#include "fractals/persist/storage.h"
#include "fractals/persist/torrent_model.h"
#include "fractals/torrent/TorrentMeta.h"
#include "fractals/common/logger.h"
#include "fractals/torrent/TorrentIO.h"

using namespace fractals::torrent;
using namespace fractals::network::http;

namespace fractals::persist {

    void save_torrent(Storage &st, std::string mi,const TorrentMeta &t) {
        auto mi_path = "./metainfo/" + t.getName() + ".torrent";
        //copy the meta info to a local directory
        std::filesystem::copy(mi,mi_path
                            ,std::filesystem::copy_options::overwrite_existing);
        auto tm = TorrentModel { 0, t.getName(), mi_path, "./downloads"};
        st.add_torrent(tm);
    }

    std::vector<TorrentMeta> load_torrents(TorrentIO &tio,Storage &st) {
        auto tms = st.load_torrents();

        std::vector<TorrentMeta> torrs;

        for(auto &tm : tms) {
            auto t = TorrentIO::readTorrentFile(tm.meta_info_path);
            if(!t.isLeft) {
                torrs.push_back(t.rightValue);
            }
        }

        return torrs;
    }

    bool has_torrent(Storage &st, const TorrentMeta &t) {
        return st.load_torrent(t.getName()).has_value();
    }

    void delete_torrent(Storage &st, const TorrentMeta &t) {
        auto tm = st.load_torrent(t.getName());
        if(tm.has_value()) {
            //delete anything that has torrent as foreign key
            delete_announces(st, t);
            delete_pieces(st, t);
            //deletes the actual torrent
            st.delete_torrent(tm.value());
        }
    }

    void save_piece(Storage &st, const TorrentMeta &t,int piece) {
        auto tm = st.load_torrent(t.getName());
        if(tm.has_value()) {
            auto pm = PieceModel {0,tm->id,piece};
            st.add_piece(pm);
        }
    }

    std::set<int> load_pieces(Storage &st, const TorrentMeta &t) {
        auto tm = st.load_torrent(t.getName());
        std::set<int> pieces;
        if(tm.has_value()) {
            auto pms = st.load_pieces(tm.value());
            
            for(auto &pm : pms) {
                pieces.insert(pm.piece);
            }
        }

        return pieces;
    }

    void delete_pieces(Storage &st,const TorrentMeta &t) {
        auto tm = st.load_torrent(t.getName());
        if(tm.has_value()) {
            auto pms = st.load_pieces(tm.value());
            for(auto &pm : pms) {
                st.delete_piece(pm);
            }
        }
    }

    void save_announce(Storage &st, const TorrentMeta &t, const Announce &ann) {
        auto tm = st.load_torrent(t.getName());
        if(tm.has_value()) {
            auto peers = ann.peers;
            std::vector<AnnounceModel> ams;
            std::transform(peers.begin(),peers.end(),std::back_inserter(ams),[&tm,&ann](PeerId &p) {
                return AnnounceModel
                    {0
                    ,tm->id
                    ,p.m_ip
                    ,p.m_port
                    ,ann.announce_time
                    ,ann.interval
                    ,ann.min_interval };
            });
            
            std::for_each(ams.begin(),ams.end(),[&st](auto &am) { st.save_announce(am); });
        }
    }

    void delete_announces(Storage &st, const TorrentMeta &t) {
        auto tm = st.load_torrent(t.getName());
        if(tm.has_value()) {
            st.delete_announces(tm.value());
        }
    }

    std::optional<Announce> load_announce(Storage &st, const TorrentMeta &t) {
        auto tm_opt = st.load_torrent(t.getName());
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

}