#include "network/p2p/BitTorrent.h"
#include "app/TorrentView.h"
#include <bits/types/time_t.h>
#include <numeric>

TorrentView::TorrentView(std::shared_ptr<BitTorrent> t) : m_model(t) {};

std::string TorrentView::get_name() {
    return m_model->m_torrent->m_name;
}

long long TorrentView::get_size() {
    auto info = m_model->m_torrent->m_mi.info; 
    if(info.file_mode.isLeft) {
        // torrent is single file therefore torrent size is equal to the size of that file
        return info.file_mode.leftValue.length;
    } else {
        auto files = info.file_mode.rightValue.files;
        // for multi mode we need to sum the size of each file
        return std::accumulate(files.begin(),files.end(),0,[](auto acc,auto &f) {
            return std::move(acc) + f.length;
        });
    }
}

long long TorrentView::get_downloaded() {
    return m_model->m_torrent->size_of_pieces(m_model->m_torrent->get_pieces());
}

long long TorrentView::get_download_speed() {
    time_t curr = std::time(0);
    if(!prev_time.has_value() || prev_time == curr) {
        prev_time = curr;
        return 0;
    }

    time_t prev = prev_time.value();

    auto downl = get_downloaded();

    // (number of bytes downloaded since previous update) / (time passed since previous update)

    auto speed = (downl - prev_downloaded) / (curr - prev);

    // update the 'prev' variables with values of 'curr' variables
    prev_downloaded = downl;
    prev_time = curr;

    return speed;
}

long long TorrentView::get_upload_speed() {
    time_t curr = std::time(0);
    if(!prev_time.has_value() || prev_time == curr) {
        prev_time = curr;
        return 0;
    }

    time_t prev = prev_time.value();

    auto upl = 0;

    // (number of bytes uploaded since previous update) / (time passed since previous update)
    auto speed = (upl - prev_uploaded) / (curr - prev);

    // update the 'prev' variables with values of 'curr' variables
    prev_uploaded = upl;
    prev_time = curr;

    return speed;
}

int TorrentView::get_total_seeders() {
    return 0;
}

int TorrentView::get_connected_seeders() {
    return m_model->m_connected;
}

int TorrentView::get_total_leechers() {
    return 0;
}

int TorrentView::get_connected_leechers() {
    return 0;
}
