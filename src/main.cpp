// #include "ftxui/screen/string.hpp"
// #include "persist/data.h"
// #include "torrent/Torrent.h"
// #include "network/p2p/BitTorrent.h"
// #include "common/logger.h"
// #include "persist/storage.h"
// #include "sqlite_orm/sqlite_orm.h"

// using namespace boost::asio;


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

// class InputBox : public ComponentBase {
//  public:
//   ~InputBox() override = default;
 
//   ftxui::Element Render() override {
//     ftxui::Elements children;
//     for (size_t i = std::max(0, (int)keys.size() - 20); i < keys.size(); ++i) {
//       children.push_back(text(Stringify(keys[i])));
//     }
//     return window(text(L"keys"), vbox(std::move(children)));
//   }
 
//   bool OnEvent(Event event) override {
//     keys.push_back(event);
//     return true;
//   }
 
//  private:
//   std::vector<Event> keys;
// };

class MyInputBase : public ComponentBase {
    private:
        StringRef content_;
        ConstStringRef placeholder_;

        bool OnMouseEvent(Event);
        Box input_box_;
        Box cursor_box_;

    public:
        // Access this interface from a Component
        static MyInputBase* From(Component component);

        // Constructor.
        MyInputBase(StringRef content, ConstStringRef placeholder);
        ~MyInputBase() override = default;

        // State.
        int cursor_position = 0;

        // State update callback.
        std::function<void()> on_change = [] {};
        std::function<void()> on_enter = [] {};
        std::function<void()> on_escape = [] {};

        // Component implementation.
        Element Render() override;
        bool OnEvent(Event) override;
};

Component MyInput(StringRef content, ConstStringRef placeholder) {
    return Make<MyInputBase>(content, placeholder);
}
  
 // static
 MyInputBase* MyInputBase::From(Component component) {
    return static_cast<MyInputBase*>(component.get());
 }
  
 MyInputBase::MyInputBase(StringRef content, ConstStringRef placeholder)
    : content_(content), placeholder_(placeholder) {}
  
 // Component implementation.
Element MyInputBase::Render() {
    cursor_position =
       std::max(0, std::min<int>(content_->size(), cursor_position));
    auto main_decorator = flex | size(HEIGHT, EQUAL, 1);
    bool is_focused = Focused();
    
    // // placeholder.
    // if (content_->size() == 0) {
    //     if (is_focused)
    //     return text(*placeholder_) | focus | dim | main_decorator |
    //             reflect(input_box_);
    //     else
    //     return text(*placeholder_) | dim | main_decorator | reflect(input_box_);
    // }
    
    // Not focused.
    if (!is_focused)
        return text(*content_) | main_decorator | reflect(input_box_);
    
    std::wstring part_before_cursor = content_->substr(0, cursor_position);

    // clang-format off
    return
        hbox(
            text(L"> "),
            text(part_before_cursor),
            text(L"\u2588")
        ) | flex | frame | main_decorator | bold | color(Color::Blue) | reflect(input_box_);
    // clang-format on
}
  
bool MyInputBase::OnEvent(Event event) {
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

    if(event == Event::Escape) {
        on_escape();
        return true;
    }
  
    if (event == Event::Custom) {
        return false;
    }
  
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
  
bool MyInputBase::OnMouseEvent(Event event) {
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

int main(int argc, const char* argv[]) {
    using namespace ftxui;
    
    printf("%c[?1049h%c[1;1H",0x1B,0x1B);

    std::wstring first_name_;
    std::wstring entered_first = L"";
    auto screen = ScreenInteractive::Fullscreen();
    Component input_first_name_ = MyInput(&first_name_, "");
    MyInputBase::From(input_first_name_)->on_enter = [&] {
        entered_first = first_name_;
        first_name_ = L"";
    };

    MyInputBase::From(input_first_name_)->on_escape = screen.ExitLoopClosure();

    auto component = Container::Vertical({
        input_first_name_,
    });
    
    auto renderer = Renderer(component, [&] {
        return vbox({
                hbox({text(L"Torrent Name") | center ,separator(),text(L"Downloaded") | center}),
                vbox({hbox({text(entered_first) | flex})}) | border | flex,
                separator(),
                hbox({input_first_name_->Render()}),
            }) |
            border | bold | color(Color::Blue);
    });
    
    
    screen.Loop(renderer);

    printf("%c[?1049l",0x1B);
}


// int main() {
//     Storage storage;
//     storage.open_storage("torrents.db");
//     storage.sync_schema();

//     //get current time and set as seed for rand
//     srand ( time(NULL) );

//     //sets attributes such as timestamp, thread id
//     boost::log::add_common_attributes();

//     //set log file as logging destination
//     boost::log::add_file_log(
//         boost::log::keywords::file_name = "log.txt",
//         //this second parameter is necessary to force the log file
//         //to be named 'log.txt'. It is a known bug in boost1.74
//         // https://github.com/boostorg/log/issues/104
//         boost::log::keywords::target_file_name = "log.txt",
//         boost::log::keywords::auto_flush = true,
//         boost::log::keywords::format = "[%ThreadID%][%TimeStamp%] %Message%"
//     );

//     // std::shared_ptr<std::mutex> mu = make_shared<std::mutex>();
//     // std::unique_lock<std::mutex> lock(*mu.get());

//     auto torr = Torrent::read_torrent("/home/ferdinand/dev/Fractals/examples/shamanking.torrent");
//     save_torrent(storage,torr);
//     auto torr_ptr = std::make_shared<Torrent>(torr);

//     boost::asio::io_context io;
//     auto bt = BitTorrent(torr_ptr,io,storage);

//     try {
//         bt.run();
//     }
//     catch(std::exception e) {
//         std::cout << e.what() << std::endl;
//     }
// }
