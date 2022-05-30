//
// Created by Ferdinand on 5/27/2022.
//

#pragma once

#include <optional>
#include <mutex>

#include "fractals/persist/storage.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::network::http { struct Announce; struct Tracker; }

namespace fractals::network::p2p {

    class AnnounceService {

        public:
            AnnounceService( persist::Storage &st
                            ,const torrent::TorrentMeta &tm);

            /**
             * Performs announce with a tracker
             * @return
             */
            std::optional<http::Announce> announce();

            void saveAnnounce(const http::Announce &ann);

        private:
            torrent::TorrentMeta mTorrentMeta;
            std::mutex mMutex;
            persist::Storage &mStorage;

            std::optional<uint32_t> mInterval;
            std::optional<std::chrono::time_point<std::chrono::system_clock>> mLastAnnounceTime;

            /**
             * Checks local database for recent announce
             * @return
             */
            std::optional<http::Announce> loadAnnounce();

            /**
             * Looks at interval time and last announce timestamp to determine
             * if a new announce can be made
             * @return
             */
            bool isAnnounceAllowed() const;

            uint32_t getInterval(const http::Announce &ann) const;
    };

}
