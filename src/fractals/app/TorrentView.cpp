#include <ctime>
#include <climits>
#include <numeric>
#include <vector>
#include <utility>

#include <neither/either.hpp>

#include "fractals/app/TorrentView.h"
#include "fractals/network/p2p/BitTorrent.h"
#include <fractals/torrent/MetaInfo.h>
#include "fractals/torrent/Torrent.h"
#include "fractals/torrent/Piece.h"

namespace fractals::app {

    TorrentView::TorrentView(int id,std::shared_ptr<network::p2p::BitTorrent> t) : m_id(id),m_model(t) {};

    std::string TorrentView::get_name() {
        return m_model->m_torrent->getName();
    }

    int64_t TorrentView::get_size() {
        return m_model->m_torrent->getMeta().getSize();
    }

    int64_t TorrentView::get_downloaded() {
        return size_of_piece(m_model->m_torrent->getMeta(),m_model->m_torrent->get_pieces());
    }

    int64_t TorrentView::get_download_speed() {
        time_t curr = std::time(0);

        if(!m_prev_time.has_value() || m_prev_time == curr) {
            m_prev_time = curr;
            return m_prev_download_speed;
        }

        time_t prev = m_prev_time.value();

        auto downl = get_downloaded();

        // (number of bytes downloaded since previous update) / (time passed since previous update)
        auto speed = (downl - m_prev_downloaded) / (curr - prev);

        // update the 'prev' variables with values of 'curr' variables
        m_prev_downloaded = downl;
        m_prev_time = curr;
        m_prev_download_speed = speed;

        return speed;
    }

    int64_t TorrentView::get_upload_speed() {
        time_t curr = std::time(0);
        if(!m_prev_time.has_value() || m_prev_time == curr) {
            m_prev_time = curr;
            return m_prev_upload_speed;
        }

        time_t prev = m_prev_time.value();

        auto upl = 0;

        // (number of bytes uploaded since previous update) / (time passed since previous update)
        auto speed = (upl - m_prev_uploaded) / (curr - prev);

        // update the 'prev' variables with values of 'curr' variables
        m_prev_uploaded = upl;
        m_prev_time = curr;
        m_prev_upload_speed = speed;

        return speed;
    }

    int TorrentView::get_total_seeders() {
        return m_model->available_peers() + m_model->connected_peers();
    }

    int TorrentView::get_connected_seeders() {
        return m_model->connected_peers();
    }

    int TorrentView::get_total_leechers() {
        return 0;
    }

    int TorrentView::get_connected_leechers() {
        return 0;
    }

    int64_t TorrentView::get_eta() {
        auto total = get_size();
        auto downloaded = get_downloaded();
        auto speed = get_download_speed();

        if (speed <= 0) {
            return LLONG_MAX;
        }

        return (total - downloaded) / speed;
    }

}