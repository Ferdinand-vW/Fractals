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

    class TorrentDisplayBase : public ftxui::ComponentBase {
        private:
            ftxui::Component m_terminal_input;
            Feedback m_feedback;

        public:
            std::map<int,TorrentView> m_running;
            std::map<int,TorrentView> m_completed;
            std::map<int,TorrentView> m_stopped;

            std::function<neither::Either<std::string,std::string>(std::string)> m_on_add;
            std::function<neither::Either<std::string,std::string>(int)> m_on_remove;
            std::function<neither::Either<std::string,std::string>(int)> m_on_stop;
            std::function<neither::Either<std::string,std::string>(int)> m_on_resume;

            // Constructor.
            TorrentDisplayBase(ftxui::Component terminal_input);
            ~TorrentDisplayBase() override = default;

            // Access this interface from a Component
            static TorrentDisplayBase* From(ftxui::Component component);
            ftxui::Element Render() override;
            bool parse_command(ftxui::StringRef ws);
    };

    ftxui::Component TorrentDisplay(ftxui::Component terminal_input);

}