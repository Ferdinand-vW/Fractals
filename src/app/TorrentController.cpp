#include "app/TorrentController.h"
#include "app/TorrentDisplay.h"
#include "ftxui/component/screen_interactive.hpp"
#include "neither/either.hpp"
#include "network/p2p/BitTorrent.h"
#include "persist/data.h"
#include "persist/storage.h"
#include <boost/asio/io_context.hpp>
#include <filesystem>
#include <functional>
#include <mutex>

TorrentController::TorrentController(boost::asio::io_context &io,Storage &st) 
                                  : m_io(io),m_storage(st),m_lg(logger::get())
                                  , m_work(work_guard(m_io.get_executor()))
                                  , m_screen(ftxui::ScreenInteractive::Fullscreen())
                                  , m_ticker(m_screen) {}

void TorrentController::run() {
    //initialize view components
    Component terminal_input = TerminalInput(&m_terminal_input, "");
    m_terminal = terminal_input;
    m_display = TorrentDisplay(terminal_input);

    for( unsigned int i = 0; i < m_thread_count; i++ ) {
        m_threads.create_thread(
            [&]() { m_io.run(); }
        );
    }

    //load known torrents
    auto torrs = load_torrents(m_storage);
    for(auto &torr : torrs) {
        BOOST_LOG(m_lg) << "loading torrent " << torr->m_name;
        add_torrent(to_bit_torrent(torr));
    }

    runUI();
}

void TorrentController::exit() {
    stop_torrents();
    m_work.reset(); //release m_io.run from threads
    m_threads.join(); //wait for torrents to be fully stopped
    m_io.stop();

    //Appears that the terminal output library cleans up after a screen loop exit
    //we need to make sure that we remove any dependencies to components owned by this class
    //before the library attempts to clean up the components when we still have an existing pointer to one
    //removal of this line may cause segfaults
    m_ticker.stop();
    m_display->reset();
    m_terminal->reset();
}

Either<std::string,TorrentName> TorrentController::on_add(std::string filepath) {
    auto eth_torr = Torrent::read_torrent(filepath);
    if(eth_torr.isLeft) {
        //provide error message to view
        return left<std::string>(eth_torr.leftValue);
    } else {
        auto torr = std::move(eth_torr.rightValue);
        if(has_torrent(m_storage, torr)) {
            return left<std::string>("torrent " + torr.m_name + " already exists!");
        }
        //write torrent to database
        save_torrent(m_storage, filepath,torr);

        //add torrent to program state
        auto bt = to_bit_torrent(torr);
        add_torrent(bt);

        //provide name of torrent to view
        return right<TorrentName>(bt->m_torrent->m_name);
    }
}

void TorrentController::add_torrent(std::shared_ptr<BitTorrent> bt) {
    int torr_id = list_torrent(bt); //adds torrent to controller state
    start_torrent(torr_id); //Start running the torrent

    //adds torrent to display
    auto tdb = TorrentDisplayBase::From(m_display.value());
    tdb->m_running.insert({torr_id,(TorrentView(torr_id,bt))});
}

Either<std::string,TorrentName> TorrentController::on_remove(int torr_id) {
    //starting a non-existing torrent does not do anything
    if(m_torrents.find(torr_id) == m_torrents.end()) {
        return left<std::string>("torrent identifier " + std::to_string(torr_id) + " does not match any torrent");
    }

    auto bt = m_torrents[torr_id];
    bt->stop();
    auto &torr = *bt->m_torrent.get();
    delete_torrent(m_storage, torr);
    m_torrents.erase(torr_id);

    auto tdb = TorrentDisplayBase::From(m_display.value());
    auto remove_in_map = [torr_id](auto &m) {
        if(m.find(torr_id) != m.end()) {
            m.erase(torr_id);
        }
    };
    //since there should only be one of each torr_id in a single map
    //we can simply attempt to delete from all. Only exactly one entry will be removed.
    remove_in_map(tdb->m_completed);
    remove_in_map(tdb->m_running);
    remove_in_map(tdb->m_stopped);

    return right<std::string>(bt->m_torrent->m_name);
}

Either<std::string,TorrentName> TorrentController::on_stop(int torr_id) {
    //starting a non-existing torrent does not do anything
    if(m_torrents.find(torr_id) == m_torrents.end()) {
        return left("torrent identifier " + std::to_string(torr_id) + " does not match any torrent");
    }

    auto bt = m_torrents[torr_id];
    bt->stop();

    auto tdb = TorrentDisplayBase::From(m_display.value());
    auto run_entry = tdb->m_running.find(torr_id);
    if(run_entry == tdb->m_running.end()) {
        return left("torrent " + std::to_string(torr_id) + " is not currently running");
    }

    //move torrent from m_stopped to m_running
    tdb->m_stopped.insert({torr_id,run_entry->second});
    tdb->m_running.erase(torr_id);

    return right<TorrentName>(bt->m_torrent->m_name);
}

Either<std::string,TorrentName> TorrentController::on_resume(int torr_id) {
    //starting a non-existing torrent does not do anything
    if(m_torrents.find(torr_id) == m_torrents.end()) {
        return left("torrent identifier " + std::to_string(torr_id) + " does not match any torrent");
    }

    auto bt = m_torrents[torr_id];
    start_torrent(torr_id);

    auto tdb = TorrentDisplayBase::From(m_display.value());
    auto stop_entry = tdb->m_stopped.find(torr_id);
    if(stop_entry == tdb->m_stopped.end()) {
        return left("torrent " + std::to_string(torr_id) + " is not currently running");
    }

    //move torrent from m_running to m_stopped
    tdb->m_running.insert({torr_id,stop_entry->second});
    tdb->m_stopped.erase(torr_id);

    return right<TorrentName>(bt->m_torrent->m_name);
}

int TorrentController::list_torrent(std::shared_ptr<BitTorrent> torrent) {
    int torr_id = -1;
    {   //make sure each displayed torrent is assigned a unique id during lifetime the program 
        std::unique_lock<std::mutex> lock(m_mutex);
        torr_id = m_torrent_counter;
        m_torrent_counter++;
    }

    // insert torrent in torrent list
    m_torrents.insert({torr_id,torrent});

    return torr_id;
}

void TorrentController::start_torrent(int torr_id) {
    //starting a non-existing torrent does not do anything
    if(m_torrents.find(torr_id) == m_torrents.end()) {
        return;
    }

    auto bt = m_torrents[torr_id];
    m_io.post([bt]() {bt->run();});
}

void TorrentController::stop_torrents() {
    for(auto &bt : m_torrents) {
        bt.second->stop();
    }
}

void TorrentController::on_exit() {
    exit();
}

void TorrentController::runUI() {
    using namespace ftxui;

    auto terminal = m_terminal.value();
    auto tdb = TorrentDisplayBase::From(m_display.value());
    
    //Sets up control flow of View -> Controller
    tdb->m_on_add    = std::bind(&TorrentController::on_add,this,std::placeholders::_1);
    tdb->m_on_remove = std::bind(&TorrentController::on_remove,this,std::placeholders::_1);
    tdb->m_on_stop   = std::bind(&TorrentController::on_stop,this,std::placeholders::_1);
    tdb->m_on_resume = std::bind(&TorrentController::on_resume,this,std::placeholders::_1);

    auto doExit = m_screen.ExitLoopClosure(); //had to move this outside of the on_enter definition
                                            // as it would otherwise not trigger
    TerminalInputBase::From(terminal)->on_escape = [this,&doExit] () {
        exit();
        doExit();
    };
    TerminalInputBase::From(terminal)->on_enter = [this,&doExit] () {
        bool shouldExit = TorrentDisplayBase::From(m_display.value())->parse_command(m_terminal_input);
        if(shouldExit) { exit(); doExit(); }
        m_terminal_input = L"";
    };

    auto renderer = Renderer(terminal,[&] { return m_display.value()->Render(); });
    m_ticker.start();
    m_screen.Loop(renderer);
}

std::shared_ptr<BitTorrent> TorrentController::to_bit_torrent(Torrent &torr) {
    auto torr_shared = std::make_shared<Torrent>(torr);
    return to_bit_torrent(torr_shared);
}

std::shared_ptr<BitTorrent> TorrentController::to_bit_torrent(std::shared_ptr<Torrent> torr) {
    return std::shared_ptr<BitTorrent>(new BitTorrent(torr,m_io,m_storage));
}