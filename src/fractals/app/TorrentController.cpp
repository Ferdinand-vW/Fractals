#include <chrono>
#include <ctime>
#include <functional>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <utility>
#include <vector>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <neither/either.hpp>

#include <fractals/app/AppEventQueue.h>
#include <fractals/app/Event.h>
#include <fractals/app/Feedback.h>
#include <fractals/app/TerminalInput.h>
#include <fractals/app/TorrentController.h>
#include <fractals/app/TorrentDisplay.h>
#include <fractals/app/TorrentDisplayEntry.h>
#include <fractals/common/Tagged.h>
#include <fractals/common/logger.h>
#include <fractals/common/utils.h>
#include <fractals/persist/Event.h>
#include <fractals/persist/PersistEventQueue.h>

namespace fractals::app
{

TorrentController::TorrentController(app::AppEventQueue::RightEndPoint btQueue,
                                     persist::AppPersistQueue::LeftEndPoint persistQueue)
    : btQueue(btQueue), persistQueue(persistQueue),
      screen(ftxui::ScreenInteractive::Fullscreen()), ticker(screen)
{
}

void TorrentController::run()
{
    // initialize view components
    Component terminalInput = TerminalInput(&terminalInputText, "");
    terminal = terminalInput;
    display = TorrentDisplay(terminalInput, idTorrentMap);

    // load known torrents
    persistQueue.push(persist::LoadTorrents{});

    runUI();
}

void TorrentController::exit()
{
    btQueue.push(app::Shutdown{});
}

void TorrentController::addTorrent(std::string filepath)
{
    btQueue.push(app::AddTorrent{filepath});
}

void TorrentController::processBtEvent(const app::AddedTorrent &event)
{
    spdlog::info("TC::processBtEvent AddedTorrent={}", event.infoHash);
    idCounter++;
    auto [it, _] = idTorrentMap.emplace(idCounter,
                                        TorrentDisplayEntry(idCounter, event.torrent));
    hashToIdMap.emplace(event.infoHash, idCounter);

    if (!it->second.isDownloadComplete())
    {
        it->second.setRunning();
        btQueue.push(app::StartTorrent{event.infoHash, event.torrent, event.files});
    }
    else
    {
        it->second.setCompleted();
    }
}

void TorrentController::processBtEvent(const app::AddTorrentError &err)
{
    spdlog::info("TC::processBtEvent AddedTorrentError={}", err.error);
    TorrentDisplayBase::From(display.value())
        ->setFeedBack(Feedback{FeedbackType::Warning, err.error});
}

void TorrentController::processBtEvent(const app::RemovedTorrent &rt)
{
    spdlog::info("TC::processBtEvent RemovedTorrent={}", rt.infoHash);
}

void TorrentController::processBtEvent(const app::StoppedTorrent &resp)
{
    spdlog::info("TC::processBtEvent StoppedTorrent={}", resp.infoHash);
    const auto torrId = hashToIdMap[resp.infoHash];
    auto &torr = idTorrentMap[torrId];
    torr.setStopped();
}

void TorrentController::processBtEvent(const app::CompletedTorrent &resp)
{
    spdlog::info("TC::processBtEvent CompletedTorrent={}", resp.infoHash);
    const auto torrId = hashToIdMap[resp.infoHash];
    auto &torr = idTorrentMap[torrId];
    torr.setCompleted();
}

void TorrentController::processBtEvent(const app::ResumedTorrent &resp)
{
    spdlog::info("TC::processBtEvent ResumedTorrent={}", resp.infoHash);
    const auto torrId = hashToIdMap[resp.infoHash];
    auto &torr = idTorrentMap[torrId];
    torr.setRunning();
}

void TorrentController::processBtEvent(const app::ShutdownConfirmation &)
{
    // Appears that the terminal output library cleans up after a screen loop exit
    // we need to make sure that we remove any dependencies to components owned by this class
    // before the library attempts to clean up the components when we still have an existing pointer
    // to one removal of this line may cause segfaults
    ticker.stop();
    display->reset();
    terminal->reset();
}

void TorrentController::processBtEvent(const app::PeerStats & peerStats)
{
    auto it = idTorrentMap.find(peerStats.torrId);
    if (it != idTorrentMap.end())
    {
        it->second.update(peerStats);
    }
}

void TorrentController::processPersistEvent(const persist::TorrentStats &stats)
{
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto it = idTorrentMap.find(stats.torrId);
    if (it != idTorrentMap.end())
    {
        it->second.update(now, stats);
    }
}

void TorrentController::processPersistEvent(const persist::AllTorrents &loadedTorrs)
{
    spdlog::info("TC::processPersistEvent(loadedTorrents) numTorrents={}",
                 loadedTorrs.result.size());
    for (const auto &torr : loadedTorrs.result)
    {
        const auto &torrModel = torr.first;
        const auto &filesModel = torr.second;
        common::InfoHash infoHash{torrModel.infoHash};

        idCounter++;
        auto [it, _] = idTorrentMap.emplace(idCounter,
                                            TorrentDisplayEntry(idCounter, torrModel));
        hashToIdMap.emplace(infoHash, it->first);

        if (it->second.isDownloadComplete())
        {
            it->second.setCompleted();
        }
        else
        {
            it->second.setRunning();
            btQueue.push(app::StartTorrent{infoHash, torrModel, filesModel});
        }
    }
}

void TorrentController::removeTorrent(uint64_t torrentId)
{
    const auto it = idTorrentMap.find(torrentId);

    if (it != idTorrentMap.end())
    {
        hashToIdMap.erase(it->second.getInfoHash());
        idTorrentMap.erase(torrentId);
        btQueue.push(app::RemoveTorrent{it->second.getInfoHash()});
    }
}

void TorrentController::readResponses()
{
    while (btQueue.canPop())
    {
        std::visit(common::overloaded{[this](const auto &resp)
                                      {
                                          processBtEvent(resp);
                                      }},
                   btQueue.pop());
    }

    while (persistQueue.canPop())
    {
        std::visit(common::overloaded{[this](const auto &resp)
                                      {
                                          processPersistEvent(resp);
                                      }},
                   persistQueue.pop());
    }
}

void TorrentController::refreshStats()
{
    std::vector<std::pair<uint64_t, common::InfoHash>> hashes;
    hashes.reserve(idTorrentMap.size());

    for (const auto &pair : idTorrentMap)
    {
        hashes.emplace_back(pair.first, pair.second.getInfoHash());
    }
    persistQueue.push(persist::RequestStats{hashes});
    btQueue.push(app::RequestStats{hashes});
}

void TorrentController::stopTorrent(uint64_t torrentId)
{
    const auto it = idTorrentMap.find(torrentId);

    if (it != idTorrentMap.end())
    {
        btQueue.push(app::StopTorrent{it->second.getInfoHash()});
    }
}

void TorrentController::resumeTorrent(uint64_t torrentId)
{
    const auto it = idTorrentMap.find(torrentId);

    if (it != idTorrentMap.end())
    {
        btQueue.push(app::ResumeTorrent{it->second.getInfoHash()});
    }
}

void TorrentController::runUI()
{
    using namespace ftxui;

    auto trmnl = terminal.value();
    auto tdb = TorrentDisplayBase::From(display.value());

    // Sets up control flow of View -> Controller
    tdb->onAdd = std::bind(&TorrentController::addTorrent, this, std::placeholders::_1);
    tdb->onRemove = std::bind(&TorrentController::removeTorrent, this, std::placeholders::_1);
    tdb->onStop = std::bind(&TorrentController::stopTorrent, this, std::placeholders::_1);
    tdb->onResume = std::bind(&TorrentController::resumeTorrent, this, std::placeholders::_1);

    auto doExit = screen.ExitLoopClosure(); // had to move this outside of the on_enter definition
    // as it would otherwise not trigger. Not sure why though..
    TerminalInputBase::From(trmnl)->onEscape = [this, &doExit]()
    {
        exit();
        doExit();
    };
    TerminalInputBase::From(trmnl)->onEnter = [this, &doExit]()
    {
        bool shouldExit =
            TorrentDisplayBase::From(display.value())->parseCommand(terminalInputText);
        if (shouldExit)
        {
            exit();
            doExit();
        }
        terminalInputText = "";
    };

    auto renderer = Renderer(trmnl,
                             [&]
                             {
                                 return display.value()->Render();
                             });
    ticker.start();
    Loop loop(&screen, renderer);

    uint64_t loopCounter{0};
    static constexpr auto tenMs = std::chrono::milliseconds(10);
    auto now = std::chrono::high_resolution_clock::now() + tenMs;
    while (!loop.HasQuitted())
    {
        screen.RequestAnimationFrame();
        loop.RunOnce();

        if (loopCounter % 10 == 0)
        {
            readResponses();
            
        }
        if (loopCounter % 100 == 0)
        {
            refreshStats();
        }

        std::this_thread::sleep_until(now);
        now += tenMs;
        ++loopCounter;
    }
}

} // namespace fractals::app
