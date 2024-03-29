#pragma once

#include <optional>
#include <thread>

namespace ftxui { class ScreenInteractive; }

namespace fractals::app {

    /**
    Class for managing the UI.
    Ensures the screen object is updated every so often. Updates can also be stopped from occurring.
    */
    class ScreenTicker {
        /**
        The actual screen object from FTXUI library
        */
        ftxui::ScreenInteractive &screen;
        /**
        The thread responsible for updating the screen object
        */
        std::optional<std::thread> ticker;
        bool isRunning = false;

        public:
            ScreenTicker(ftxui::ScreenInteractive &screen);
            ~ScreenTicker();
            void start();
            void stop();
    };

}