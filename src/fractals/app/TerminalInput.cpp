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
        cursorPosition =
        std::max(0, std::min<int>(content_->size(), cursorPosition));
        auto mainDecorator = flex | size(HEIGHT, EQUAL, 1);
        bool isFocused = Focused();
        
        // Not focused.
        if (!isFocused)
            return text(*content_) | mainDecorator | reflect(inputBox);
        
        std::string partBeforeCursor = content_->substr(0, cursorPosition);

        return
            hbox(
                text(L"> "),
                text(partBeforeCursor),
                text(L"\u2588") // block character
            ) | flex | frame | mainDecorator | bold | color(Color::Blue) | reflect(inputBox);
            // sadly blinking didn't seem to work
    }
    
    bool TerminalInputBase::OnEvent(Event event) {
        cursorPosition =
        std::max(0, std::min<int>(content_->size(), cursorPosition));
    
        if (event.is_mouse())
            return OnMouseEvent(event);
    
        std::string c;
    
        // Backspace.
        if (event == Event::Backspace) {
            if (cursorPosition == 0)
                return false;
            content_->erase(cursorPosition - 1, 1);
            cursorPosition--;
            onChange();
            return true;
        }
        
        // Delete
        if (event == Event::Delete) {
            if (cursorPosition == int(content_->size()))
                return false;
            content_->erase(cursorPosition, 1);
            onChange();
            return true;
        }
        
        // Enter.
        if (event == Event::Return) {
            onEnter();
            return true;
        }

        // Escape.
        if(event == Event::Escape) {
            onEscape();
            return true;
        }
    
        if (event == Event::Custom) {
            return false;
        }
    
        // Arrow keys
        if (event == Event::ArrowLeft && cursorPosition > 0) {
            cursorPosition--;
            return true;
        }
    
        if (event == Event::ArrowRight && cursorPosition < (int)content_->size()) {
            cursorPosition++;
            return true;
        }
    
        if (event == Event::Home) {
            cursorPosition = 0;
            return true;
        }
    
        if (event == Event::End) {
            cursorPosition = (int)content_->size();
            return true;
        }
    
        // Content
        if (event.is_character()) {
            content_->insert(cursorPosition, event.character()); // (cursor_position, 1, event.character())
            cursorPosition++;
            onChange();
            return true;
        }
    return false;
    }
    
    bool TerminalInputBase::OnMouseEvent(Event event) {
        if (!CaptureMouse(event))
            return false;
        if (!inputBox.Contain(event.mouse().x, event.mouse().y))
            return false;
        
        if (event.mouse().button == Mouse::Left &&
            event.mouse().motion == Mouse::Pressed) {
            TakeFocus();
            int newCursorPosition =
                cursorPosition + event.mouse().x - cursorBox.x_min;
            newCursorPosition =
                std::max(0, std::min<int>(content_->size(), newCursorPosition));
            if (cursorPosition != newCursorPosition) {
            cursorPosition = newCursorPosition;
            onChange();
            }
        }
        return true;
    }

}