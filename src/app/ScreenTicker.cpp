#include <thread>
#include <chrono>

#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "app/ScreenTicker.h"

ScreenTicker::ScreenTicker(ftxui::ScreenInteractive &screen) : m_screen(screen) {};
ScreenTicker::~ScreenTicker() {
    m_running = false;
    m_ticker->join();
    m_ticker.reset();
}

void ScreenTicker::start() {
    if(!m_running) {
        m_running = true;
        m_ticker = std::thread([this]() {
            while(m_running) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(0.05s);
                m_screen.PostEvent(ftxui::Event::Custom);
            }
        });
    }
}

void ScreenTicker::stop() {
    m_running = false;
}