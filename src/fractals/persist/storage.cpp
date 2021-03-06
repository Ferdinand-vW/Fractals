#include <mutex>
#include <optional>

#include <sqlite_orm/sqlite_orm.h>

#include "fractals/common/utils.h"
#include "fractals/persist/announce_model.h"
#include "fractals/persist/piece_model.h"
#include "fractals/persist/storage.h"
#include "fractals/persist/torrent_model.h"

namespace fractals::persist {

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

    void Storage::add_torrent(const TorrentModel &tm) {
        std::unique_lock<std::mutex> l(m_update_mutex);
        m_storage->insert(tm);
    }

    std::optional<TorrentModel> Storage::load_torrent(std::string name) {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> l(m_update_mutex);
        auto tms = m_storage->get_all<TorrentModel>(
                where(c(&TorrentModel::name) == name)
        );

        common::print_err("load torrent"+std::to_string(tms.size()));
        if(tms.begin() != tms.end()) {
            return tms.front();
        } else {
            return {};
        }
    }

    std::vector<TorrentModel> Storage::load_torrents() {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> l(m_update_mutex);
        return m_storage->get_all<TorrentModel>();

    }

    void Storage::delete_torrent(const TorrentModel &t) {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> _lock(m_update_mutex);
        m_storage->remove<TorrentModel>(t.id);
    }

    void Storage::add_piece(const PieceModel &pm) {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> _lock(m_update_mutex);
        m_storage->insert(pm);
    }

    std::vector<PieceModel> Storage::load_pieces(const TorrentModel &t) {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> l(m_update_mutex);
        return m_storage->get_all<PieceModel>(
            where(c(&PieceModel::torrent_id) == t.id)
        );
    }

    void Storage::delete_piece(const PieceModel &pm) {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> l(m_update_mutex);
        m_storage->remove<PieceModel>(pm.id);
    }

    void Storage::save_announce(const AnnounceModel &ann) {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> _lock(m_update_mutex);
        m_storage->insert(ann);
    }

    void Storage::delete_announces(const TorrentModel &tm) {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> _lock(m_update_mutex);
        m_storage->remove_all<AnnounceModel>(
            where(c(&AnnounceModel::torrent_id) == tm.id)
        );
    }

    std::vector<AnnounceModel> Storage::load_announce(const TorrentModel &t) {
        using namespace sqlite_orm;
        std::unique_lock<std::mutex> l(m_update_mutex);
        return m_storage->get_all<AnnounceModel>(
            where(c(&AnnounceModel::torrent_id) == t.id)
        );
    }

}