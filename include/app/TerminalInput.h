#pragma once

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

class TerminalInputBase : public ComponentBase {
    private:
        StringRef content_;
        ConstStringRef placeholder_;

        bool OnMouseEvent(ftxui::Event);
        Box input_box_;
        Box cursor_box_;

    public:
        // Access this interface from a Component
        static TerminalInputBase* From(Component component);

        // Constructor.
        TerminalInputBase(StringRef content, ConstStringRef placeholder);
        ~TerminalInputBase() override = default;

        // State.
        int cursor_position = 0;

        // State update callback.
        std::function<void()> on_change = [] {};
        std::function<void()> on_enter = [] {};
        std::function<void()> on_escape = [] {};

        // Component implementation.
        Element Render() override;
        bool OnEvent(ftxui::Event) override;
};

Component TerminalInput(StringRef content, ConstStringRef placeholder);
