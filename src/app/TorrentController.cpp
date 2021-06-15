#include "app/TorrentController.h"
#include "ftxui/component/screen_interactive.hpp"
#include "neither/either.hpp"
#include "persist/storage.h"
#include <boost/asio/io_context.hpp>

TorrentController::TorrentController(boost::asio::io_context &io,Storage &st) 
                                  : m_io(io),m_storage(st)
                                  , m_screen(ftxui::ScreenInteractive::Fullscreen()) {}

void TorrentController::run() {
    runUI();
}

Either<std::string,int> TorrentController::on_add(std::string filepath) {
    return left("test"s);
}

std::optional<std::string> on_remove(int torr_id) {
    return {};
}

std::optional<std::string> on_stop(int torr_id) {
    return {};
}

std::optional<std::string> on_resume(int torr_id) {
    return {};  
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