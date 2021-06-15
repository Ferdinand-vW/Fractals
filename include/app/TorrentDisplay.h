#pragma once

#include "network/p2p/BitTorrent.h"
#include "torrent/Torrent.h"
#include "app/TerminalInput.h"
#include "app/Feedback.h"
#include "app/TorrentView.h"

#include "ftxui/component/input.hpp"
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"       // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, hbox, Element, separator, operator|, vbox, border
#include "ftxui/dom/elements.hpp"              // for Element
#include "ftxui/screen/box.hpp"                // for Box
#include "ftxui/screen/color.hpp"
#include "ftxui/screen/string.hpp"             // for ConstStringRef, StringRef

#include "ftxui/component/captured_mouse.hpp"  // for CapturedMouse
#include "ftxui/component/event.hpp"  // for Event, Event::ArrowLeft, Event::ArrowRight, Event::Backspace, Event::Custom, Event::Delete, Event::End, Event::Home, Event::Return
#include "ftxui/component/input.hpp"
#include "ftxui/component/mouse.hpp"  // for Mouse, Mouse::Left, Mouse::Pressed
#include "ftxui/component/screen_interactive.hpp"  // for Component

using namespace ftxui;

class TorrentDisplayBase : public ComponentBase {
    private:
        std::vector<TorrentView> m_running;
        std::vector<TorrentView> m_completed;
        std::vector<TorrentView> m_stopped;
        Component m_terminal_input;
        Feedback m_feedback;

    public:
        // Constructor.
        TorrentDisplayBase(Component terminal_input);
        ~TorrentDisplayBase() override = default;

        // Access this interface from a Component
        static TorrentDisplayBase* From(Component component);
        Element Render() override;
        bool parse_command(StringRef ws);
};

Component TorrentDisplay(Component terminal_input);