#include "app/TorrentController.h"
#include "app/TorrentDisplay.h"
#include "ftxui/component/screen_interactive.hpp"
#include "neither/either.hpp"
#include "network/p2p/BitTorrent.h"
#include "persist/data.h"
#include "persist/storage.h"
#include <boost/asio/io_context.hpp>
#include <functional>
#include <mutex>

TorrentController::TorrentController(boost::asio::io_context &io,Storage &st) 
                                  : m_io(io),m_storage(st),m_lg(logger::get())
                                  , m_work(work_guard(m_io.get_executor()))
                                  , m_screen(ftxui::ScreenInteractive::Fullscreen()) {}

void TorrentController::run() {
    for( unsigned int i = 0; i < m_thread_count; i++ ) {
        m_threads.create_thread(
            [&]() { m_io.run(); }
        );
    }

    runUI();
}

void TorrentController::exit() {
    stop_torrents();
    time_t curr = std::time(0);
    m_work.reset();
    m_threads.join(); //wait for torrents to be fully stopped
    time_t curr2 = std::time(0);
    BOOST_LOG(m_lg) << "hallo" << curr << " " << curr2;
    m_io.stop();
    //this doesn't actually do anything
    //for some reason we need to exit the loop from within runUI function
    m_screen.ExitLoopClosure();
}

Either<std::string,std::string> TorrentController::on_add(std::string filepath) {
    auto torr = Torrent::read_torrent(filepath);
    if(torr.isLeft) {
        return left<std::string>(torr.leftValue);
    } else {
        //write torrent to database
        save_torrent(m_storage, torr.rightValue);

        auto torr_shared = std::make_shared<Torrent>(torr.rightValue);
        auto bt = std::shared_ptr<BitTorrent>(new BitTorrent(torr_shared,m_io,m_storage));

        //list torrent in display and assign id
        int torr_id = list_torrent(bt);
        start_torrent(torr_id); //Start running the torrent

        auto tdb = TorrentDisplayBase::From(m_display);
        tdb->m_stopped.push_back(TorrentView(bt));

        return right<std::string>(torr_shared->m_name);
    }
}

std::optional<std::string> TorrentController::on_remove(int torr_id) {
    return {};
}

std::optional<std::string> TorrentController::on_stop(int torr_id) {
    return {};
}

std::optional<std::string> TorrentController::on_resume(int torr_id) {
    return {};  
}

int TorrentController::list_torrent(std::shared_ptr<BitTorrent> torrent) {
    int torr_id = -1;
    {   //make sure each displayed torrent is assigned a unique id during lifetime the program 
        std::unique_lock<std::mutex> lock(m_mutex);
        torr_id = m_torrent_counter;
        m_torrent_counter++;
    }

    // display the 
    m_torrents.insert({torr_id,torrent});

    return torr_id;
}

void TorrentController::start_torrent(int torr_id) {
    //starting a non-existing torrent does not do anything
    if(m_torrents.find(torr_id) == m_torrents.end()) {
        return;
    }

    auto bt = m_torrents[torr_id];
    bt->run();
}

void TorrentController::stop_torrents() {
    for(auto &bt : m_active_torrents) {
        bt.second->stop();
    }

    for(auto &bt : m_torrents) {
        bt.second->stop();
    }
}

void TorrentController::on_exit() {
    exit();
}

void TorrentController::runUI() {
    using namespace ftxui;

    std::wstring input_string;
    Component terminal_input = TerminalInput(&input_string, "");

    //set up control of view for controller
    m_display = TorrentDisplay(terminal_input);
    auto tdb = TorrentDisplayBase::From(m_display);
    
    //Sets up control flow of View -> Controller
    tdb->m_on_add    = std::bind(&TorrentController::on_add,this,std::placeholders::_1);
    tdb->m_on_remove = std::bind(&TorrentController::on_remove,this,std::placeholders::_1);
    tdb->m_on_stop   = std::bind(&TorrentController::on_stop,this,std::placeholders::_1);
    tdb->m_on_resume = std::bind(&TorrentController::on_resume,this,std::placeholders::_1);

    auto doExit = m_screen.ExitLoopClosure(); //had to move this outside of the on_enter definition
                                            // as it would otherwise not trigger
    TerminalInputBase::From(terminal_input)->on_escape = [this,&doExit] () {
        exit();
        doExit();
    };
    TerminalInputBase::From(terminal_input)->on_enter = [this,&doExit,&input_string] () {
        bool shouldExit = TorrentDisplayBase::From(m_display)->parse_command(input_string);
        if(shouldExit) { exit(); doExit(); }
        input_string = L"";
    };

    auto renderer = Renderer(terminal_input,[&] { return m_display->Render(); });
    m_screen.Loop(renderer);
}