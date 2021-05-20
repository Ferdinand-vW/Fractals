#pragma once

#include <string>
#include <memory>

#include "torrent/Torrent.h"
#include "network/p2p/Announce.h"
#include "torrent_model.h"
#include "piece_model.h"
#include "announce_model.h"
#include "announce_peer_model.h"
#include "sqlite_orm/sqlite_orm.h"

inline auto init_storage(std::string db) {
    using namespace sqlite_orm;
    return make_storage("torrents.db",
                                    make_table("torrent",
                                            make_column("id", &TorrentModel::id, primary_key()),
                                            make_column("name", &TorrentModel::name),
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
                                            make_column("datetime",&AnnounceModel::datetime),
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
        std::shared_ptr<InternalStorage> storage;

        Storage();
        void open_storage(std::string db);
        void sync_schema();

        void add_torrent(Torrent t);
        std::vector<Torrent> load_torrents();
        void delete_torrent(Torrent t);

        void add_piece(Torrent t,int piece);
        std::vector<int> load_pieces(Torrent t);

        void add_announce(Torrent t,Announce ann);
        std::optional<Announce> load_announce(Torrent t);

};
