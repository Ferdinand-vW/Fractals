#pragma once

#include <optional>
#include <thread>

namespace ftxui { class ScreenInteractive; }

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