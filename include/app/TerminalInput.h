#pragma once

#include <functional>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>  // for ComponentBase
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>                // for Box
#include <ftxui/screen/string.hpp>             // for ConstStringRef, StringRef

namespace ftxui { struct Event; }

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
