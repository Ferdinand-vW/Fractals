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

    //load known torrents
    auto torrs = load_torrents(m_storage);
    for(auto &torr : torrs) {
        BOOST_LOG(m_lg) << torr->m_name;
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
    m_display.reset();
    
    //this doesn't actually do anything
    //for some reason we need to exit the loop from within runUI function
    // m_screen.ExitLoopClosure();
}

Either<std::string,std::string> TorrentController::on_add(std::string filepath) {
    auto torr = Torrent::read_torrent(filepath);
    if(torr.isLeft) {
        //provide error message to view
        return left<std::string>(torr.leftValue);
    } else {
        auto bt = to_bit_torrent(torr.rightValue);
        auto res = add_torrent(bt);

        if(res.has_value()) {
            return left<std::string>(res.value());
        } else {
            //provide name of torrent to view
            return right<std::string>(bt->m_torrent->m_name);
        }
    }
}

std::optional<std::string> TorrentController::add_torrent(std::shared_ptr<BitTorrent> bt) {
    if(has_torrent(m_storage, *bt->m_torrent.get())) {
        return "torrent " + bt->m_torrent->m_name + " already exists!";
    }

    //write torrent to database
    save_torrent(m_storage, *bt->m_torrent.get());

    int torr_id = list_torrent(bt); //adds torrent to controller state
    start_torrent(torr_id); //Start running the torrent

    //adds torrent to display
    auto tdb = TorrentDisplayBase::From(m_display.value());
    tdb->m_stopped.push_back(TorrentView(bt));

    return {};
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
    bt->run();
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

    std::wstring input_string;
    Component terminal_input = TerminalInput(&input_string, "");

    //set up control of view for controller
    m_display = TorrentDisplay(terminal_input);
    auto tdb = TorrentDisplayBase::From(m_display.value());
    
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
        bool shouldExit = TorrentDisplayBase::From(m_display.value())->parse_command(input_string);
        if(shouldExit) { exit(); doExit(); }
        input_string = L"";
    };

    auto renderer = Renderer(terminal_input,[&] { return m_display.value()->Render(); });
    m_screen.Loop(renderer);
}

std::shared_ptr<BitTorrent> TorrentController::to_bit_torrent(Torrent &torr) {
    auto torr_shared = std::make_shared<Torrent>(torr);
    return to_bit_torrent(torr_shared);
}

std::shared_ptr<BitTorrent> TorrentController::to_bit_torrent(std::shared_ptr<Torrent> torr) {
    return std::shared_ptr<BitTorrent>(new BitTorrent(torr,m_io,m_storage));
}