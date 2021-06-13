#include "app/UI.h"
#include "app/TerminalInput.h"
#include "app/TorrentDisplay.h"
#include <cstdio>

void runUI() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    std::wstring input_string;
    Component terminal_input = TerminalInput(&input_string, "");

    Component td = TorrentDisplay(terminal_input);
    auto doExit = screen.ExitLoopClosure();
    TerminalInputBase::From(terminal_input)->on_escape = doExit;
    TerminalInputBase::From(terminal_input)->on_enter = [&td,&doExit,&input_string] () {
        bool shouldExit = TorrentDisplayBase::From(td)->parse_command(input_string);
        if(shouldExit) { doExit(); }
        input_string = L"";
    };

    auto renderer = Renderer(terminal_input,[&] { return td->Render(); });
    screen.Loop(renderer);
}