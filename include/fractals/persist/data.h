#pragma once

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace fractals::torrent { class Torrent; }
namespace fractals::network::http { class Announce; }

namespace fractals::persist {

    class Storage;

    void save_torrent(Storage &st, std::string mi,const torrent::Torrent &t);
    std::vector<std::shared_ptr<torrent::Torrent>> load_torrents(Storage &st);
    bool has_torrent(Storage &st, const torrent::Torrent &t);
    void delete_torrent (Storage &st,const torrent::Torrent &t);

    void save_piece(Storage &st,const torrent::Torrent &t,int piece);
    std::set<int> load_pieces(Storage &st,const torrent::Torrent &t);
    void delete_pieces(Storage &st,const torrent::Torrent &t);

    void save_announce(Storage &st,const torrent::Torrent &t,const network::http::Announce &ann);
    void delete_announces(Storage &st,const torrent::Torrent &t);
    std::optional<network::http::Announce> load_announce(Storage &st,const torrent::Torrent &t);

}