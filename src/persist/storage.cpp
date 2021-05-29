#include "persist/storage.h"
#include "persist/torrent_model.h"
#include "sqlite_orm/sqlite_orm.h"
#include "torrent/Torrent.h"
#include <optional>

Storage::Storage() {};

void Storage::open_storage(std::string db) {
    if(m_storage == nullptr || !m_storage->is_opened()) {
        using namespace sqlite_orm;
        //connect to user given db and create tables if not already present
        auto storage = init_storage(db);
        // update shared pointer to point to new database connection
        m_storage = std::make_shared<InternalStorage>(storage);                                        
    }
}

void Storage::sync_schema() {
    m_storage->sync_schema();
}

void Storage::add_torrent(const TorrentModel &tm) const {
    m_storage->insert(tm);
}

std::optional<TorrentModel> Storage::load_torrent(std::string name) const {
    using namespace sqlite_orm;
    auto tms = m_storage->get_all<TorrentModel>(
            where(c(&TorrentModel::name) == name)
    );

    if(tms.begin() != tms.end()) {
        return tms.front();
    } else {
        return {};
    }
}

std::vector<TorrentModel> Storage::load_torrents() const {
    using namespace sqlite_orm;
    return m_storage->get_all<TorrentModel>();
}

void Storage::delete_torrent(const TorrentModel &t) const {
    using namespace sqlite_orm;
    m_storage->remove<TorrentModel>(where(c(&TorrentModel::id) == t.id));
}

void Storage::add_piece(const PieceModel &pm) const {
    using namespace sqlite_orm;
    m_storage->insert(pm);
}

std::vector<PieceModel> Storage::load_pieces(const TorrentModel &t) const {
    using namespace sqlite_orm;
    return m_storage->get_all<PieceModel>(
        where(c(&PieceModel::torrent_id) == t.id)
    );
}