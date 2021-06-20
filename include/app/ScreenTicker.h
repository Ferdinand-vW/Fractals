#pragma once

#include "ftxui/component/screen_interactive.hpp"
#include <thread>

class ScreenTicker {
    ftxui::ScreenInteractive &m_screen;
    std::optional<std::thread> m_ticker;
    bool m_running = false;

    public:
        ScreenTicker(ftxui::ScreenInteractive &screen);
        ~ScreenTicker();
        void start();
        void stop();
};