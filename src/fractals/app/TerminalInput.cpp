#include <algorithm>
#include <string>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/string.hpp>

#include "fractals/app/TerminalInput.h"

namespace fractals::app {

    Component TerminalInput(StringRef content, ConstStringRef placeholder) {
        return Make<TerminalInputBase>(content, placeholder);
    }

    TerminalInputBase* TerminalInputBase::From(Component component) {
        return static_cast<TerminalInputBase*>(component.get());
    }

    TerminalInputBase::TerminalInputBase(StringRef content, ConstStringRef placeholder)
        : content_(content), placeholder_(placeholder) {}

    // Component implementation.
    Element TerminalInputBase::Render() {
        cursor_position =
        std::max(0, std::min<int>(content_->size(), cursor_position));
        auto main_decorator = flex | size(HEIGHT, EQUAL, 1);
        bool is_focused = Focused();
        
        // Not focused.
        if (!is_focused)
            return text(*content_) | main_decorator | reflect(input_box_);
        
        std::wstring part_before_cursor = content_->substr(0, cursor_position);

        return
            hbox(
                text(L"> "),
                text(part_before_cursor),
                text(L"\u2588") // block character
            ) | flex | frame | main_decorator | bold | color(Color::Blue) | reflect(input_box_);
            // sadly blinking didn't seem to work
    }
    
    bool TerminalInputBase::OnEvent(Event event) {
        cursor_position =
        std::max(0, std::min<int>(content_->size(), cursor_position));
    
        if (event.is_mouse())
            return OnMouseEvent(event);
    
        std::wstring c;
    
        // Backspace.
        if (event == Event::Backspace) {
            if (cursor_position == 0)
                return false;
            content_->erase(cursor_position - 1, 1);
            cursor_position--;
            on_change();
            return true;
        }
        
        // Delete
        if (event == Event::Delete) {
            if (cursor_position == int(content_->size()))
                return false;
            content_->erase(cursor_position, 1);
            on_change();
            return true;
        }
        
        // Enter.
        if (event == Event::Return) {
            on_enter();
            return true;
        }

        // Escape.
        if(event == Event::Escape) {
            on_escape();
            return true;
        }
    
        if (event == Event::Custom) {
            return false;
        }
    
        // Arrow keys
        if (event == Event::ArrowLeft && cursor_position > 0) {
            cursor_position--;
            return true;
        }
    
        if (event == Event::ArrowRight && cursor_position < (int)content_->size()) {
            cursor_position++;
            return true;
        }
    
        if (event == Event::Home) {
            cursor_position = 0;
            return true;
        }
    
        if (event == Event::End) {
            cursor_position = (int)content_->size();
            return true;
        }
    
        // Content
        if (event.is_character()) {
            content_->insert(cursor_position, 1, event.character());
            cursor_position++;
            on_change();
            return true;
        }
    return false;
    }
    
    bool TerminalInputBase::OnMouseEvent(Event event) {
        if (!CaptureMouse(event))
            return false;
        if (!input_box_.Contain(event.mouse().x, event.mouse().y))
            return false;
        
        if (event.mouse().button == Mouse::Left &&
            event.mouse().motion == Mouse::Pressed) {
            TakeFocus();
            int new_cursor_position =
                cursor_position + event.mouse().x - cursor_box_.x_min;
            new_cursor_position =
                std::max(0, std::min<int>(content_->size(), new_cursor_position));
            if (cursor_position != new_cursor_position) {
            cursor_position = new_cursor_position;
            on_change();
            }
        }
        return true;
    }

}