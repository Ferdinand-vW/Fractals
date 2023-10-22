#pragma once

#include "fractals/app/Event.h"
#include "fractals/app/TorrentDisplayEntry.h"
#include "fractals/common/Tagged.h"
#include "fractals/persist/Event.h"
#include "fractals/torrent/TorrentMeta.h"
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <ftxui/component/screen_interactive.hpp>
#include <neither/either.hpp>

#include <fractals/app/ScreenTicker.h>
#include <fractals/app/AppEventQueue.h>
#include <fractals/persist/PersistEventQueue.h>

namespace fractals::app
{

typedef std::string TorrentName;

/**
Main controller class for app.
Responsibilities include:
 - Keep track of active p2p connections
 - Manage work threads
 - Handle app user interactions
 - Run the UI thread
*/
class TorrentController
{
  public:
    TorrentController(app::AppEventQueue::RightEndPoint, persist::AppPersistQueue::LeftEndPoint);
    void run();
    void exit();

  private:
    void runUI();

    void addTorrent(std::string filepath);
    void removeTorrent(uint64_t torrentId);
    void stopTorrent(uint64_t torrentId);
    void resumeTorrent(uint64_t torrentId);

    void readResponses();
    void refreshStats();

    void processBtEvent(const app::AddedTorrent&);
    void processBtEvent(const app::AddTorrentError&);
    void processBtEvent(const app::RemovedTorrent&);
    void processBtEvent(const app::StoppedTorrent&);
    void processBtEvent(const app::CompletedTorrent&);
    void processBtEvent(const app::ResumedTorrent&);
    void processBtEvent(const app::ShutdownConfirmation&);
    void processBtEvent(const app::PeerStats&);

    void processPersistEvent(const persist::TorrentStats&);
    void processPersistEvent(const persist::AllTorrents&);

  private:
    app::AppEventQueue::RightEndPoint btQueue;
    persist::AppPersistQueue::LeftEndPoint persistQueue;

    /**
    Counter used to assign unique id to each torrent.
    The id of a torrent is visible in the '#' column.
    */
    int idCounter = 0;

    std::optional<ftxui::Component> display;
    std::optional<ftxui::Component> terminal;
    std::string terminalInputText;

    ftxui::ScreenInteractive screen;
    ScreenTicker ticker;

    std::unordered_map<common::InfoHash, uint64_t> hashToIdMap;
    std::unordered_map<uint64_t, TorrentDisplayEntry> idTorrentMap;
};

} // namespace fractals::app