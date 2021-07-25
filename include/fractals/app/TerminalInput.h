#pragma once

#include <functional>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>  // for ComponentBase
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>                // for Box
#include <ftxui/screen/string.hpp>             // for ConstStringRef, StringRef

namespace ftxui { struct Event; }

namespace fractals::app {

    using namespace ftxui;

    /**
    Class for managing the (terminal) input box.
    Commands provided by the user are parsed and acted upon by this class.
    */
    class TerminalInputBase : public ComponentBase {
        private:
            StringRef content_;
            ConstStringRef placeholder_;

            bool OnMouseEvent(ftxui::Event);
            Box input_box_;
            Box cursor_box_;

        public:
            /**
            Access existing TerminalInputBase instance from a Component.
            */
            static TerminalInputBase* From(Component component);

            TerminalInputBase(StringRef content, ConstStringRef placeholder);
            ~TerminalInputBase() override = default;

            int cursor_position = 0;

            /**
            Callbacks on user interactions.
            */ 
            std::function<void()> on_change = [] {};
            std::function<void()> on_enter = [] {};
            std::function<void()> on_escape = [] {};

            /**
            Required implementation of ComponentBase
            */
            Element Render() override;
            bool OnEvent(ftxui::Event) override;
    };

    /**
    Function to create new instances of @TerminalInputBase@.
    */
    Component TerminalInput(StringRef content, ConstStringRef placeholder);

}