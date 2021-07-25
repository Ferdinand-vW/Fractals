#pragma once

#include <functional>
#include <map>
#include <string>

#include <ftxui/component/component_base.hpp>  // for ComponentBase
#include <ftxui/dom/elements.hpp>

#include "fractals/app/Feedback.h"
#include "fractals/app/TorrentView.h"

namespace neither { template <class L, class R> struct Either; }
namespace ftxui {class StringRef; }

namespace fractals::app {

    /**
    UI component for displaying BitTorrent connections
    */
    class TorrentDisplayBase : public ftxui::ComponentBase {
        private:
            ftxui::Component m_terminal_input;
            Feedback m_feedback;

        public:
            /**
            Torrents can be either runniing, are completed or have been stopped by user
            */
            std::map<int,TorrentView> m_running;
            std::map<int,TorrentView> m_completed;
            std::map<int,TorrentView> m_stopped;

            /**
            Functions to be called when certain user interactions happen
            */
            std::function<neither::Either<std::string,std::string>(std::string)> m_on_add;
            std::function<neither::Either<std::string,std::string>(int)> m_on_remove;
            std::function<neither::Either<std::string,std::string>(int)> m_on_stop;
            std::function<neither::Either<std::string,std::string>(int)> m_on_resume;

            TorrentDisplayBase(ftxui::Component terminal_input);
            ~TorrentDisplayBase() override = default;

            /**
            Accept instance of TorrentDisplayBase from Component
            */
            static TorrentDisplayBase* From(ftxui::Component component);
            
            /**
            ComponentBase implementation
            */
            ftxui::Element Render() override;
            bool parse_command(ftxui::StringRef ws);
    };

    /**
    Function to be called when constructing instance of TorrentDisplayBase
    */
    ftxui::Component TorrentDisplay(ftxui::Component terminal_input);

}