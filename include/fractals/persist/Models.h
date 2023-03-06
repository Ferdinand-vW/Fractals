#pragma once

#include <sqlite_orm/sqlite_orm.h>

namespace fractals::persist
{
/**
ADT for announce model in database
*/
struct AnnounceModel
{
    int id;
    int torrent_id;
    std::string peer_ip;
    uint16_t peer_port;
    time_t announce_time;
    int interval;
    std::optional<int> min_interval;
    bool tested = false; // have we tried connecting to a peer?
};

/**
ADT for piece model in database
*/
struct PieceModel
{
    int id;
    int torrent_id;
    uint32_t piece;
};

/**
ADT for torrent model in database
*/
struct TorrentModel
{
    int id;
    std::string name;
    std::string meta_info_path;
    std::string write_path;
};

/**
Defines database model.

Defined here such that we can construct a type alias (e.g. 'InternalStorage') for the 'storage' type
*/
inline auto makeDatabase(std::string db)
{
    using namespace sqlite_orm;
    return make_storage(
        db,
        make_table("torrent", make_column("id", &TorrentModel::id, primary_key()),
                   make_column("name", &TorrentModel::name, unique()),
                   make_column("meta_info_file", &TorrentModel::meta_info_path),
                   make_column("write_path", &TorrentModel::write_path)),
        make_table("torrent_piece", make_column("id", &PieceModel::id, primary_key()),
                   make_column("torrent_id", &PieceModel::torrent_id), make_column("piece", &PieceModel::piece),
                   foreign_key(&PieceModel::torrent_id).references(&TorrentModel::id)),
        make_table("torrent_announce", make_column("id", &AnnounceModel::id, primary_key()),
                   make_column("torrent_id", &AnnounceModel::torrent_id),
                   make_column("peer_ip", &AnnounceModel::peer_ip), make_column("peer_port", &AnnounceModel::peer_port),
                   make_column("announce_time", &AnnounceModel::announce_time),
                   make_column("interval", &AnnounceModel::interval),
                   make_column("min_interval", &AnnounceModel::min_interval),
                   foreign_key(&AnnounceModel::torrent_id).references(&TorrentModel::id)));
}
// namespace

using Schema = decltype(makeDatabase(""));

} // namespace fractals::persist