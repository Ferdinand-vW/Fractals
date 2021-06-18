#include "app/TorrentController.h"
#include "app/TorrentDisplay.h"
#include "ftxui/component/screen_interactive.hpp"
#include "neither/either.hpp"
#include "persist/data.h"
#include "persist/storage.h"
#include <boost/asio/io_context.hpp>
#include <functional>
#include <mutex>

TorrentController::TorrentController(boost::asio::io_context &io,Storage &st) 
                                  : m_io(io),m_storage(st)
                                  , m_screen(ftxui::ScreenInteractive::Fullscreen()) {}

void TorrentController::run() {
    runUI();
}

Either<std::string,int> TorrentController::on_add(std::string filepath) {
    auto torr = Torrent::read_torrent(filepath);
    if(torr.isLeft) {
        return left<std::string>(torr.leftValue);
    } else {
        //write torrent to database
        save_torrent(m_storage, torr.rightValue);

        //list torrent in display and assign id
        int torr_id = list_torrent(std::make_shared<Torrent>(torr.rightValue));
        start_torrent(torr_id); //Start running the torrent

        return right<int>(torr_id);
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

int TorrentController::list_torrent(std::shared_ptr<Torrent> torrent) {
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

}

void TorrentController::on_exit() {
    
    //stop active torrents

    m_screen.ExitLoopClosure();
}

void TorrentController::runUI() {
    using namespace ftxui;

    std::wstring input_string;
    Component terminal_input = TerminalInput(&input_string, "");

    Component td = TorrentDisplay(terminal_input);
    auto tdb = TorrentDisplayBase::From(td);

    //Sets up control flow of View -> Controller
    tdb->on_add    = std::bind(&TorrentController::on_add,this,std::placeholders::_1);
    tdb->on_remove = std::bind(&TorrentController::on_remove,this,std::placeholders::_1);
    tdb->on_stop   = std::bind(&TorrentController::on_stop,this,std::placeholders::_1);
    tdb->on_resume = std::bind(&TorrentController::on_resume,this,std::placeholders::_1);

    auto doExit = m_screen.ExitLoopClosure(); //had to move this outside of the on_enter definition
                                            // as it would otherwise not trigger
    TerminalInputBase::From(terminal_input)->on_escape = doExit;
    TerminalInputBase::From(terminal_input)->on_enter = [&td,&doExit,&input_string] () {
        bool shouldExit = TorrentDisplayBase::From(td)->parse_command(input_string);
        if(shouldExit) { doExit(); }
        input_string = L"";
    };

    auto renderer = Renderer(terminal_input,[&] { return td->Render(); });
    m_screen.Loop(renderer);
}