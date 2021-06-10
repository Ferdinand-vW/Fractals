#include "app/UI.h"
#include "app/TorrentDisplay.h"
#include <cstdio>

void runUI() {
    printf("%c[?1049h%c[1;1H",0x1B,0x1B);

    auto screen = ftxui::ScreenInteractive::Fullscreen();

    std::wstring input_string;
    Component terminal_input = TerminalInput(&input_string, "");

    Component td = TorrentDisplay(terminal_input);
    TerminalInputBase::From(terminal_input)->on_enter = [&] {
        TorrentDisplayBase::From(td)->parse_command(input_string);
        input_string = L"";
    };

    auto renderer = Renderer(terminal_input,[&] { return td->Render(); });

    screen.Loop(renderer);

    printf("%c[?1049l",0x1B);
}