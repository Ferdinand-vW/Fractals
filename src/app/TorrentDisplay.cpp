#include "app/UI.h"
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <sstream>

Component TorrentDisplay(Component terminal_input) {
    return Make<TorrentDisplayBase>(terminal_input);
}

TorrentDisplayBase::TorrentDisplayBase(Component terminal_input) : m_terminal_input(terminal_input) {};

TorrentDisplayBase* TorrentDisplayBase::From(Component component) {
    return static_cast<TorrentDisplayBase*>(component.get());
}

std::optional<std::wstring> TorrentDisplayBase::parse_command(std::wstring ws) {
    std::wstringstream ss(ws);
    std::wstring com;
    ss >> com;
    
    m_output_string = L"gotcha";

    if(com.compare(L"exit")) {
        return {};
    }

    if(!elem(com,L"add",L"stop",L"remove")) {
        return L"unknown command: " + com;
    }

    if(ss.rdbuf()->in_avail()) {
        return L"expecting exactly 1 argument but got none";
    }

    if(com.compare(L"add")) {
        std::wstring path;
        ss >> path;
        if(!std::filesystem::exists(path)) {
            return L"path does not exist: " + path;
        } else {
            //parse file and create torrent
            // on error return error message
            return {};
        }
    } else if (com.compare(L"stop")) {
        std::wstring torr_id;
        ss >> torr_id;
        return {};
    } else {

        return {};
    }
}

Element TorrentDisplayBase::Render() {
    auto column = [](std::wstring ws) { return text(ws) | color(Color::Red) | hcenter; };
    auto cell = [](std::wstring ws) { return text(ws) | color(Color::Blue) | xflex; };
    auto cols = [](Elements && e) { return vbox({e}) | xflex; };
    auto colOfSize = [column](std::wstring name,int i) {
            int rem = i - name.length();
            int b_size = std::floor(rem / 2);
            int a_size = std::ceil(rem / 2); 
            std::string before(b_size,' ');
            std::string after(a_size,' ');
            std::wstring wbefore(before.begin(),before.end());
            std::wstring wafter(after.begin(),after.end());
            return hbox({text(wbefore),column(name),text(wafter)});
        };

    return vbox({
                // columns
                hbox({
                    vbox({
                        column(L"#"),
                        separator() | color(Color::GreenLight),
                        cell(L"test"),
                        cell(L"test2")}),
                    vbox({separator(),filler(),filler()}),
                    vbox({
                        column(L"State"),
                        separator() | color(Color::GreenLight),
                        cell(L"Running"),
                        cell(L"Completed")
                    }),
                    vbox({separator(),filler(),filler()}),
                    cols({
                        column(L"Torrent Name"),
                        separator() | color(Color::GreenLight),
                        cell(L"test"),
                        cell(L"test2")}),
                    vbox({separator(),filler(),filler()}),
                    vbox({
                        colOfSize(L"Size", 12),
                        separator() | color(Color::GreenLight),
                        cell(L"test"),
                        cell(L"test2")}),
                    vbox({separator(),filler(),filler()}),
                    vbox({
                        colOfSize(L"Progress", 16),
                        separator() | color(Color::GreenLight),
                        cell(L"test"),
                        cell(L"test")}),
                    vbox({separator(),filler(),filler()}),
                    vbox({
                        colOfSize(L"Down", 12),
                        separator() | color(Color::GreenLight),
                        cell(L"test"),
                        cell(L"test")}),
                    vbox({separator(),filler(),filler()}),
                    vbox({
                        colOfSize(L"Up", 12),
                        separator() | color(Color::GreenLight),
                        cell(L"test"),
                        cell(L"test2")}),
                    vbox({separator(),filler(),filler()}),
                    cols({
                        column(L"ETA"),
                        separator() | color(Color::GreenLight),
                        cell(L""),
                        cell(L"")})
                        })  | flex ,
                separator() | color(Color::GreenLight),
                hbox({text(m_output_string)}),
                separator() | color(Color::GreenLight),
                hbox({m_terminal_input->Render()})  | color(Color::GreenLight),
                separator()  | color(Color::GreenLight)
            }) |
            bold | flex | color(Color::Blue);
}