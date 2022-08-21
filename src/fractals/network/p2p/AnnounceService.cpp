//
// Created by Ferdinand on 5/28/2022.
//

#include <memory>

#include "fractals/common/logger.h"
#include "fractals/network/http/Tracker.h"
#include "fractals/network/http/Request.h"
#include "fractals/persist/data.h"
#include "fractals/network/p2p/AnnounceService.h"

namespace fractals::network::p2p {

    AnnounceService::AnnounceService(persist::Storage &st
                                    ,const torrent::TorrentMeta &tm)
                                    : mStorage(st),mTorrentMeta(tm)
                                    , mTracker(std::make_unique<network::http::TrackerClient>(network::http::TrackerClient(tm.getMetaInfo().announce))){};

    std::optional<http::Announce> AnnounceService::announce() {
        std::unique_lock<std::mutex> lck(mMutex,std::defer_lock);
        if(!lck.try_lock()) { return {}; } //Some thread is already handling announce

        // Make sure we can't announce again if we performed a recent announce
        if(!isAnnounceAllowed()) { return {}; }

        // See if there exists a recent announce in storage
        auto recentAnnOpt = loadAnnounce();

        if (recentAnnOpt.has_value()) {
            // TODO: Add information about clock time when announce in database was made
            mInterval = getInterval(recentAnnOpt.value());

            return recentAnnOpt;
        }

        // Start new announce with a tracker
        auto tr = http::makeTrackerRequest(mTorrentMeta.getMetaInfo());
        time_t curr = std::time(0);
        auto resp = mTracker->sendRequest(tr);

        if(resp.isLeft) {
            BOOST_LOG(common::logger::get()) << "[BitTorrent] tracker response error: " << resp.leftValue;
            if (resp.leftValue == "announcing too fast") { sleep(10); }
            return {};
        }
        else {
            BOOST_LOG(common::logger::get()) << "[BitTorrent] tracker response: " << resp.rightValue;
            auto ann = resp.rightValue.toAnnounce(curr);

            mInterval = getInterval(ann);
            auto now = std::chrono::system_clock::now();
            mLastAnnounceTime = now;

            return ann;
        }
    }

    std::optional<http::Announce> AnnounceService::loadAnnounce() {
        auto mann = persist::load_announce(mStorage, mTorrentMeta);
        if(mann.has_value()) {
            auto ann = mann.value();
            time_t curr = std::time(0);
            int iv = ann.min_interval.value_or(ann.interval);
            //if a recent announce exists then return those peers
            //we should not announce more often than @min_interval@
            if (curr - ann.announce_time < iv) {
                BOOST_LOG(common::logger::get()) << "[BitTorrent] recent announce exists";
                return ann;
            } else {
                //return nothing since the announce stored in database is not recent enough
                return {};
            }
        }

        return {};
    }

    bool AnnounceService::isAnnounceAllowed() const {
        if(mInterval.has_value() && mLastAnnounceTime.has_value()) {
            auto now = std::chrono::system_clock::now();
            auto elapsedSeconds = now - mLastAnnounceTime.value();
            return elapsedSeconds.count() >= mInterval.value();
        }

        return true; // announce is allowed if a previous one has not been done before
    }

    uint32_t AnnounceService::getInterval(const http::Announce &ann) const {
        return ann.min_interval.value_or(ann.interval);
    }

    void AnnounceService::saveAnnounce(const http::Announce &ann) {
        delete_announces(mStorage,mTorrentMeta);
        save_announce(mStorage, mTorrentMeta, ann); //saves announce in db
    }
}