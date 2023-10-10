#pragma once

#include "fractals/common/Tagged.h"
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
    uint64_t size;
    std::vector<char> hash;
    bool complete{false};
};

struct TrackerModel
{
    int id;
    int torrent_id;
    std::string url;
};

struct FileModel
{
    int id;
    int torrent_id;
    std::string fileName;
    std::string dirName;
    uint64_t length;
    std::string md5sum;
};

/**
ADT for torrent model in database
*/
struct TorrentModel
{
    int id;
    std::string infoHash;
    std::string name;
    std::string dirName;
    std::string meta_info_path;
    std::string write_path;
    uint64_t size;
    uint64_t numPieces;
    uint64_t pieceLength;
    std::optional<int64_t> creationDate;
    std::optional<std::string> comment;
    std::optional<std::string> createdBy;
    std::optional<std::string> encoding;
    std::optional<int64_t> publish;
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
                   make_column("infoHash", &TorrentModel::infoHash, unique()),
                   make_column("name", &TorrentModel::name, unique()),
                   make_column("dirName", &TorrentModel::dirName),
                   make_column("meta_info_file", &TorrentModel::meta_info_path),
                   make_column("write_path", &TorrentModel::write_path),
                   make_column("size", &TorrentModel::size),
                   make_column("numPieces", &TorrentModel::numPieces),
                   make_column("pieceLength", &TorrentModel::pieceLength),
                   make_column("creationDate", &TorrentModel::creationDate),
                   make_column("comment", &TorrentModel::comment),
                   make_column("createdBy", &TorrentModel::createdBy),
                   make_column("encoding", &TorrentModel::encoding),
                   make_column("publish", &TorrentModel::publish)),
        make_table("file", make_column("id", &FileModel::id, primary_key()),
                   make_column("torrent_id", &FileModel::torrent_id),
                   make_column("fileName", &FileModel::fileName),
                   make_column("dirName", &FileModel::dirName),
                   make_column("length", &FileModel::length),
                   make_column("md5sum", &FileModel::md5sum),
                   foreign_key(&FileModel::torrent_id).references(&TorrentModel::id)),
        make_table("piece", make_column("id", &PieceModel::id, primary_key()),
                   make_column("torrent_id", &PieceModel::torrent_id),
                   make_column("piece", &PieceModel::piece), make_column("size", &PieceModel::size),
                   make_column("hash", &PieceModel::hash),
                   make_column("complete", &PieceModel::complete),
                   foreign_key(&PieceModel::torrent_id).references(&TorrentModel::id)),
        make_table("tracker", make_column("id", &TrackerModel::id, primary_key()),
                   make_column("torrent_id", &TrackerModel::torrent_id),
                   make_column("url", &TrackerModel::url),
                   foreign_key(&TrackerModel::torrent_id).references(&TorrentModel::id)),
        make_table("announce", make_column("id", &AnnounceModel::id, primary_key()),
                   make_column("torrent_id", &AnnounceModel::torrent_id),
                   make_column("peer_ip", &AnnounceModel::peer_ip),
                   make_column("peer_port", &AnnounceModel::peer_port),
                   make_column("announce_time", &AnnounceModel::announce_time),
                   make_column("interval", &AnnounceModel::interval),
                   make_column("min_interval", &AnnounceModel::min_interval),
                   foreign_key(&AnnounceModel::torrent_id).references(&TorrentModel::id)));
}
// namespace

using Schema = decltype(makeDatabase(""));

} // namespace fractals::persist