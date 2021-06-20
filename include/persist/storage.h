#pragma once

#include <string>
#include <memory>

#include "torrent/Torrent.h"
#include "torrent_model.h"
#include "piece_model.h"
#include "announce_model.h"
#include "announce_peer_model.h"
#include "sqlite_orm/sqlite_orm.h"

// defined here such that we can construct a type alias (e.g. 'InternalStorage') for the 'storage' type
inline auto init_storage(std::string db) {
    using namespace sqlite_orm;
    return make_storage(db,
                            make_table("torrent",
                                    make_column("id", &TorrentModel::id, primary_key()),
                                    make_column("name", &TorrentModel::name, unique()),
                                    make_column("meta_info_file", &TorrentModel::meta_info_path),
                                    make_column("write_path", &TorrentModel::write_path)),
                            make_table("torrent_piece",
                                    make_column("id", &PieceModel::id, primary_key()),
                                    make_column("torrent_id", &PieceModel::torrent_id),
                                    make_column("piece", &PieceModel::piece),
                                    foreign_key(&PieceModel::torrent_id).references(&TorrentModel::id)),
                            make_table("torrent_announce",
                                    make_column("id",&AnnounceModel::id,primary_key()),
                                    make_column("torrent_id",&AnnounceModel::torrent_id),
                                    make_column("peer_ip",&AnnounceModel::peer_ip),
                                    make_column("peer_port",&AnnounceModel::peer_port),
                                    make_column("announce_time",&AnnounceModel::announce_time),
                                    make_column("interval",&AnnounceModel::interval),
                                    make_column("min_interval",&AnnounceModel::min_interval),
                                    foreign_key(&AnnounceModel::torrent_id).references(&TorrentModel::id)),
                            make_table("announce_peer",
                                    make_column("id",&AnnouncePeerModel::id,primary_key()),
                                    make_column("announce_id",&AnnouncePeerModel::announce_id),
                                    make_column("ip",&AnnouncePeerModel::ip),
                                    make_column("port",&AnnouncePeerModel::port),
                                    foreign_key(&AnnouncePeerModel::announce_id).references(&AnnounceModel::id)));
}


using InternalStorage = decltype(init_storage(""));

class Storage {

    public:
        std::shared_ptr<InternalStorage> m_storage;

        Storage();
        void open_storage(std::string db);
        void sync_schema();

        void add_torrent(const TorrentModel &t) const;
        std::optional<TorrentModel> load_torrent(std::string name) const;
        std::vector<TorrentModel> load_torrents() const;
        void delete_torrent(const TorrentModel &t) const;

        void add_piece(const PieceModel &pm) const;
        std::vector<PieceModel> load_pieces(const TorrentModel &t) const;

        void save_announce(const AnnounceModel &ann) const;
        void delete_announces(const TorrentModel &tm) const;
        std::vector<AnnounceModel> load_announce(const TorrentModel &t) const;

};
