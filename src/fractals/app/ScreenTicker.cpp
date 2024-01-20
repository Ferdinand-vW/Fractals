#include <thread>
#include <chrono>

#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <fractals/app/ScreenTicker.h>

namespace fractals::app {

    ScreenTicker::ScreenTicker(ftxui::ScreenInteractive &screen) : screen(screen) {}
    ScreenTicker::~ScreenTicker() {
        isRunning = false;
        ticker->join();
        ticker.reset();
    }

    void ScreenTicker::start() {
        if(!isRunning) {
            isRunning = true;
            //spawn a new thread that refreshes the display every 50ms
            ticker = std::thread([this]() {
                while(isRunning) {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(0.05s);
                    screen.PostEvent(ftxui::Event::Custom);
                }
            });
        }
    }

    void ScreenTicker::stop() {
        isRunning = false;
    }

}