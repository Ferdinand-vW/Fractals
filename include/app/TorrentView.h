#pragma once

#include "network/p2p/BitTorrent.h"
#include "torrent/Torrent.h"

class TorrentView {

    public:
        int m_id;

        TorrentView(std::shared_ptr<BitTorrent> t);
        std::string get_name();
        long long get_size();
        long long get_downloaded();
        //speeds are reported in number of bytes per second
        long long get_download_speed();
        long long get_upload_speed();
        int get_total_seeders();
        int get_connected_seeders();
        int get_total_leechers();
        int get_connected_leechers();

    private:
        std::shared_ptr<BitTorrent> m_model;
        std::optional<time_t> prev_time;
        long long prev_downloaded = 0;
        long long prev_uploaded = 0;
};