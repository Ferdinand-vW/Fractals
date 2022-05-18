#pragma once

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace fractals::torrent { class TorrentMeta; class TorrentIO; }
namespace fractals::network::http { class Announce; }

namespace fractals::persist {

    class Storage;

    /**
    simple API for interacting with the given database connection 
    */

    void save_torrent(Storage &st, std::string mi,const torrent::TorrentMeta &t);
    std::vector<torrent::TorrentMeta> load_torrents(torrent::TorrentIO &tio,Storage &st);
    bool has_torrent(Storage &st, const torrent::TorrentMeta &t);
    void delete_torrent (Storage &st,const torrent::TorrentMeta &t);

    void save_piece(Storage &st,const torrent::TorrentMeta &t,int piece);
    std::set<int> load_pieces(Storage &st,const torrent::TorrentMeta &t);
    void delete_pieces(Storage &st,const torrent::TorrentMeta &t);

    void save_announce(Storage &st,const torrent::TorrentMeta &t,const network::http::Announce &ann);
    void delete_announces(Storage &st,const torrent::TorrentMeta &t);
    std::optional<network::http::Announce> load_announce(Storage &st,const torrent::TorrentMeta &t);

}