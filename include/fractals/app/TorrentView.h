#pragma once

#include <memory>
#include <optional>
#include <string>
#include <sys/types.h>

namespace fractals::network::p2p { class BitTorrent; }

namespace fractals::app {

    /**
    View of active BitTorrent connection.
    Responsible for calculating various statistics (e.g. download speed)
    */
    class TorrentView {

        public:
            int m_id;

            TorrentView(int id,std::shared_ptr<network::p2p::BitTorrent> t);
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
            long long get_eta();

        private:
            std::shared_ptr<network::p2p::BitTorrent> m_model;
            std::optional<time_t> m_prev_time;
            long long m_prev_download_speed = 0;
            long long m_prev_upload_speed = 0;
            long long m_prev_downloaded = 0;
            long long m_prev_uploaded = 0;
    };

}