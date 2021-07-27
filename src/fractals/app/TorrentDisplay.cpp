#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/string.hpp>
#include <neither/either.hpp>

#include "fractals/app/Feedback.h"
#include "fractals/app/TorrentDisplay.h"
#include "fractals/common/utils.h"

using namespace ftxui;

namespace fractals::app {

    ftxui::Component TorrentDisplay(ftxui::Component terminal_input) {
        return Make<TorrentDisplayBase>(terminal_input);
    }

    TorrentDisplayBase::TorrentDisplayBase(ftxui::Component terminal_input) : m_terminal_input(terminal_input) {};

    TorrentDisplayBase* TorrentDisplayBase::From(ftxui::Component component) {
        return static_cast<TorrentDisplayBase*>(component.get());
    }

    // @ws contains a by user written command. Here we attempt to parse it and call the relevant functions.
    bool TorrentDisplayBase::parse_command(StringRef ws) {
        std::wstringstream ss(ws->data());
        std::wstring com;
        ss >> com;
        
        if(com == L"exit") {
            m_feedback.m_msg = L"Exiting...";
            m_feedback.m_type = FeedbackType::Warning;
            return true;
        } 
        
        if(com == L"help") {
            m_feedback.m_msg = L"Available commands: exit, add <filepath>, stop <torrent name or id>, resume <torrent name or id>, remove <torrent name or id>";
            m_feedback.m_type = FeedbackType::Info;
            return false;
        }

        if(!common::elem(com,L"add",L"stop",L"resume",L"remove")) {
            m_feedback.m_msg = L"unknown command: " + com;
            m_feedback.m_type = FeedbackType::Error;

            return false;
        }

        if(!ss.rdbuf()->in_avail()) {
            m_feedback.m_msg = L"expecting exactly 1 argument but got none";
            m_feedback.m_type = FeedbackType::Error;

            return false;
        }

        //parse string identifiers with format '#<integer>'
        //the integer *should* refer to a torrent id
        //return error messages on failure
        auto parse_ident = [](auto &ident) -> neither::Either<std::string,int> {
            if(ident.substr(0,1) != L"#") {
                return neither::left<std::string>("torrent identifier must start with #. Expected format is #<torrent id>.");
            }

            auto s_num = ident.substr(1,ident.size() - 1);
            int num = -1;
            auto b = boost::conversion::try_lexical_convert<int>(s_num,num);
            if(!b) {
                return neither::left<std::string>("torrent identifier must contain number. Expected format is #<torrent id>.");
            } else {
                return neither::right<int>(num);
            }
        };

        if(com == L"add") {
            std::wstring path;
            ss >> path;
            if(!std::filesystem::exists(path)) {
                m_feedback.m_msg = L"path does not exist: " + path;
                m_feedback.m_type = FeedbackType::Error;
            } else {
                auto res = m_on_add(common::unwide(path));
                if(res.isLeft) {
                    m_feedback.m_msg = common::make_wide(res.leftValue);
                    m_feedback.m_type = FeedbackType::Error;
                } else {
                    m_feedback.m_msg = L"added torrent " + common::make_wide(res.rightValue);
                    m_feedback.m_type = FeedbackType::Info;
                }
            }

            return false;
        } 
        
        if (com == L"stop") {
            //parse and validate the second argument
            std::wstring ident;
            ss >> ident;
            auto arg_ident = parse_ident(ident);

            if(arg_ident.isLeft) {
                m_feedback.m_msg = common::make_wide(arg_ident.leftValue);
                m_feedback.m_type = FeedbackType::Error;
            } else {
                //send stop message to TorrentController
                auto res = m_on_stop(arg_ident.rightValue);
                if(res.isLeft) { //means failure
                    m_feedback.m_msg = common::make_wide(res.leftValue);
                    m_feedback.m_type = FeedbackType::Error;
                } else {
                    m_feedback.m_msg = L"paused torrent " + common::make_wide(res.rightValue);
                    m_feedback.m_type = FeedbackType::Info;
                }
            }
        }

        if(com == L"resume") {
            //parse and validate the second argument
            std::wstring ident;
            ss >> ident;
            auto arg_ident = parse_ident(ident);

            if(arg_ident.isLeft) {
                m_feedback.m_msg = common::make_wide(arg_ident.leftValue);
                m_feedback.m_type = FeedbackType::Error;
            } else {
                //send resume message to TorrentController
                auto res = m_on_resume(arg_ident.rightValue);
                if(res.isLeft) {
                    m_feedback.m_msg = common::make_wide(res.leftValue);
                    m_feedback.m_type = FeedbackType::Error;
                } else {
                    m_feedback.m_msg = L"resumed torrent " + common::make_wide(res.rightValue);
                    m_feedback.m_type = FeedbackType::Info;
                }
            }
        }

        if(com == L"remove") {
            //parse and validate the second argument
            std::wstring ident;
            ss >> ident;
            auto arg_ident = parse_ident(ident);

            if(arg_ident.isLeft) {
                m_feedback.m_msg = common::make_wide(arg_ident.leftValue);
                m_feedback.m_type = FeedbackType::Error;
            } else {
                //send remove message to TorrentController
                auto res = m_on_remove(arg_ident.rightValue);
                if(res.isLeft) {
                    m_feedback.m_msg = common::make_wide(res.leftValue);
                    m_feedback.m_type = FeedbackType::Error;
                } else {
                    m_feedback.m_msg = L"removed torrent " + common::make_wide(res.rightValue);
                    m_feedback.m_type = FeedbackType::Info;
                }
            }
        }

        return false;
    }

    Element TorrentDisplayBase::Render() {
        auto column = [](std::wstring ws) { return text(ws) | color(Color::Red) | hcenter; };
        auto cell = [](std::wstring ws) { return text(ws) | color(Color::Blue) | xflex; };
        auto cols = [](Elements && e) { return vbox({e}) | xflex; };

        //fills spaces before and after if string size smaller than int.
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

        std::vector<ftxui::Element> idElems;
        std::vector<ftxui::Element> stateElems;
        std::vector<ftxui::Element> nameElems;
        std::vector<ftxui::Element> sizeElems;
        std::vector<ftxui::Element> progressElems;
        std::vector<ftxui::Element> downElems;
        std::vector<ftxui::Element> upElems;
        std::vector<ftxui::Element> seederElems;
        std::vector<ftxui::Element> leecherElems;
        std::vector<ftxui::Element> etaElems;

        // populate the columns given a set of torrents
        auto populate = [&](auto &s,auto &collection) {
            for(auto &[_,tv] : collection) {
                idElems.push_back(cell(std::to_wstring(tv.m_id)));
                stateElems.push_back(cell(s));
                nameElems.push_back(cell(common::make_wide(tv.get_name())));
                sizeElems.push_back(cell(common::pp_bytes(tv.get_size())));
                progressElems.push_back(cell(common::pp_bytes(tv.get_downloaded())));
                downElems.push_back(cell(common::pp_bytes_per_second(tv.get_download_speed())));
                upElems.push_back(cell(common::pp_bytes_per_second(tv.get_upload_speed())));
                auto connects = [](auto act,auto tot) { 
                    return std::to_wstring(act) + L"(" + std::to_wstring(tot) + L")";
                };
                seederElems.push_back(cell(connects(tv.get_connected_seeders(),tv.get_total_seeders())));
                leecherElems.push_back(cell(connects(tv.get_connected_leechers(),tv.get_connected_leechers())));
                etaElems.push_back(cell(common::pp_time(tv.get_eta())));
            }
        };

        // set column headers
        idElems.push_back(colOfSize(L"#",4));
        idElems.push_back(separator() | color(Color::GreenLight));
        stateElems.push_back(colOfSize(L"State",10));
        stateElems.push_back(separator() | color(Color::GreenLight));
        nameElems.push_back(column(L"Torrent Name"));
        nameElems.push_back(separator() | color(Color::GreenLight));
        sizeElems.push_back(colOfSize(L"Size", 12));
        sizeElems.push_back(separator() | color(Color::GreenLight));
        progressElems.push_back(colOfSize(L"Progress", 16));
        progressElems.push_back(separator() | color(Color::GreenLight));
        downElems.push_back(colOfSize(L"Down", 12));
        downElems.push_back(separator() | color(Color::GreenLight));
        upElems.push_back(colOfSize(L"Up", 12));
        upElems.push_back(separator() | color(Color::GreenLight));
        seederElems.push_back(colOfSize(L"Seeders",12));
        seederElems.push_back(separator() | color(Color::GreenLight));
        leecherElems.push_back(colOfSize(L"Leechers",12));
        leecherElems.push_back(separator() | color(Color::GreenLight));
        etaElems.push_back(column(L"ETA"));
        etaElems.push_back(separator() | color(Color::GreenLight));

        // //populate each column; each column 1 cell per torrent
        populate(L"Running",m_running);
        populate(L"Completed",m_completed);
        populate(L"Stopped",m_stopped);
        
        return vbox({
                    // columns
                    hbox({
                        vbox(idElems),
                        vbox({separator()}),
                        vbox(stateElems),
                        vbox({separator()}),
                        vbox(nameElems) | xflex,
                        vbox({separator()}),
                        vbox(sizeElems),
                        vbox({separator()}),
                        vbox(progressElems),
                        vbox({separator()}),
                        vbox(downElems),
                        vbox({separator()}),
                        vbox(upElems),
                        vbox({separator()}),
                        vbox(seederElems),
                        vbox({separator()}),
                        vbox(leecherElems),
                        vbox({separator()}),
                        vbox(etaElems) | xflex
                        }) | flex,
                    hbox({text(m_feedback.m_msg) | color(feedBackColor(m_feedback))}),
                    separator() | color(Color::GreenLight),
                    hbox({m_terminal_input->Render()})  | color(Color::GreenLight),
                    separator()  | color(Color::GreenLight)
                }) |
                bold | flex | color(Color::Blue);
    }

}