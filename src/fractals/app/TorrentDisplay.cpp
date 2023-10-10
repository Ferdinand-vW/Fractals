#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
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
#include "fractals/app/TorrentDisplayEntry.h"
#include "fractals/common/utils.h"

using namespace ftxui;

namespace fractals::app
{

ftxui::Component TorrentDisplay(ftxui::Component terminal_input,
                                const std::unordered_map<uint64_t, TorrentDisplayEntry> &torrents)
{
    return Make<TorrentDisplayBase>(terminal_input, torrents);
}

TorrentDisplayBase::TorrentDisplayBase(
    ftxui::Component terminal_input,
    const std::unordered_map<uint64_t, TorrentDisplayEntry> &torrents)
    : m_terminal_input(terminal_input), torrents(torrents)
{
}

TorrentDisplayBase *TorrentDisplayBase::From(ftxui::Component component)
{
    return static_cast<TorrentDisplayBase *>(component.get());
}

void TorrentDisplayBase::unSetFeedback()
{
    m_feedback.m_msg = "";
    m_feedback.m_type = FeedbackType::Info;
}

void TorrentDisplayBase::setFeedBack(const Feedback& feedback)
{
    m_feedback = feedback;
}

// @ws contains a by user written command. Here we attempt to parse it and call the relevant
// functions.
bool TorrentDisplayBase::parse_command(StringRef ws)
{
    std::stringstream ss(ws->data());
    std::string com;
    ss >> com;

    if (com == "exit")
    {
        m_feedback.m_msg = "Exiting...";
        m_feedback.m_type = FeedbackType::Warning;
        return true;
    }

    if (com == "help")
    {
        m_feedback.m_msg = "Available commands: exit, add <filepath>, stop <torrent name or id>, "
                           "resume <torrent name or id>, remove <torrent name or id>";
        m_feedback.m_type = FeedbackType::Info;
        return false;
    }

    if (!common::elem(com, "add", "stop", "resume", "remove"))
    {
        m_feedback.m_msg = "unknown command: " + com;
        m_feedback.m_type = FeedbackType::Error;

        return false;
    }

    if (!ss.rdbuf()->in_avail())
    {
        m_feedback.m_msg = "expecting exactly 1 argument but got none";
        m_feedback.m_type = FeedbackType::Error;

        return false;
    }

    unSetFeedback();

    // parse string identifiers with format '#<integer>'
    // the integer *should* refer to a torrent id
    // return error messages on failure
    auto parse_ident = [](auto &ident) -> neither::Either<std::string, int>
    {
        if (ident.substr(0, 1) != "#")
        {
            return neither::left<std::string>(
                "torrent identifier must start with #. Expected format is #<torrent id>.");
        }

        auto s_num = ident.substr(1, ident.size() - 1);
        int num = -1;
        auto b = boost::conversion::try_lexical_convert<int>(s_num, num);
        if (!b)
        {
            return neither::left<std::string>(
                "torrent identifier must contain number. Expected format is #<torrent id>.");
        }
        else
        {
            return neither::right<int>(num);
        }
    };

    if (com == "add")
    {
        std::string path;
        ss >> path;
        if (!std::filesystem::exists(path))
        {
            m_feedback.m_msg = "path does not exist: " + path;
            m_feedback.m_type = FeedbackType::Error;
        }
        else
        {
            m_on_add(path);
        }

        return false;
    }

    if (com == "stop")
    {
        // parse and validate the second argument
        std::string ident;
        ss >> ident;
        auto arg_ident = parse_ident(ident);

        if (arg_ident.isLeft)
        {
            m_feedback.m_msg = arg_ident.leftValue;
            m_feedback.m_type = FeedbackType::Error;
        }
        else
        {
            // send stop message to TorrentController
            m_on_stop(arg_ident.rightValue);
        }
    }

    if (com == "resume")
    {
        // parse and validate the second argument
        std::string ident;
        ss >> ident;
        auto arg_ident = parse_ident(ident);

        if (arg_ident.isLeft)
        {
            m_feedback.m_msg = arg_ident.leftValue;
            m_feedback.m_type = FeedbackType::Error;
        }
        else
        {
            // send resume message to TorrentController
            m_on_resume(arg_ident.rightValue);
        }
    }

    if (com == "remove")
    {
        // parse and validate the second argument
        std::string ident;
        ss >> ident;
        auto arg_ident = parse_ident(ident);

        if (arg_ident.isLeft)
        {
            m_feedback.m_msg = arg_ident.leftValue;
            m_feedback.m_type = FeedbackType::Error;
        }
        else
        {
            // send remove message to TorrentController
            m_on_remove(arg_ident.rightValue);
        }
    }

    return false;
}

Element TorrentDisplayBase::Render()
{
    auto column = [](std::string ws)
    {
        return text(ws) | color(Color::Red) | hcenter;
    };
    auto cell = [](std::string ws)
    {
        return text(ws) | color(Color::Blue) | xflex;
    };
    auto cols = [](Elements &&e)
    {
        return vbox({e}) | xflex;
    };

    // fills spaces before and after if string size smaller than int.
    auto colOfSize = [column](std::string name, int i)
    {
        int rem = i - name.length();
        int b_size = std::floor(rem / 2);
        int a_size = std::ceil(rem / 2);
        std::string before(b_size, ' ');
        std::string after(a_size, ' ');
        std::string wbefore(before.begin(), before.end());
        std::string wafter(after.begin(), after.end());
        return hbox({text(wbefore), column(name), text(wafter)});
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
    auto populate = [&](const auto &s, auto state)
    {
        for (auto &[_, tv] : torrents)
        {
            if (tv.getState() != state)
            {
                continue;
            }
            idElems.push_back(cell(std::to_string(tv.getId())));
            stateElems.push_back(cell(s));
            nameElems.push_back(cell(tv.getName()));
            sizeElems.push_back(cell(common::pp_bytes(tv.getSize())));
            progressElems.push_back(cell(common::pp_bytes(tv.getDownloaded())));
            downElems.push_back(cell(common::pp_bytes_per_second(tv.getDownloadSpeed())));
            upElems.push_back(cell(common::pp_bytes_per_second(tv.getUploadSpeed())));
            auto connects = [](auto act, auto tot)
            {
                return std::to_string(act) + "(" + std::to_string(tot) + ")";
            };
            seederElems.push_back(cell(connects(tv.getConnectedSeeders(), tv.getTotalSeeders())));
            leecherElems.push_back(
                cell(connects(tv.getConnectedLeechers(), tv.getTotalLeechers())));
            etaElems.push_back(cell(common::pp_time(tv.getEta())));
        }
    };

    // set column headers
    idElems.push_back(colOfSize("#", 4));
    idElems.push_back(separator() | color(Color::GreenLight));
    stateElems.push_back(colOfSize("State", 10));
    stateElems.push_back(separator() | color(Color::GreenLight));
    nameElems.push_back(column("Torrent Name"));
    nameElems.push_back(separator() | color(Color::GreenLight));
    sizeElems.push_back(colOfSize("Size", 12));
    sizeElems.push_back(separator() | color(Color::GreenLight));
    progressElems.push_back(colOfSize("Progress", 16));
    progressElems.push_back(separator() | color(Color::GreenLight));
    downElems.push_back(colOfSize("Down", 12));
    downElems.push_back(separator() | color(Color::GreenLight));
    upElems.push_back(colOfSize("Up", 12));
    upElems.push_back(separator() | color(Color::GreenLight));
    seederElems.push_back(colOfSize("Seeders", 12));
    seederElems.push_back(separator() | color(Color::GreenLight));
    leecherElems.push_back(colOfSize("Leechers", 12));
    leecherElems.push_back(separator() | color(Color::GreenLight));
    etaElems.push_back(column("ETA"));
    etaElems.push_back(separator() | color(Color::GreenLight));

    // //populate each column; each column 1 cell per torrent
    populate("Running", TorrentDisplayEntry::State::Running);
    populate("Completed", TorrentDisplayEntry::State::Completed);
    populate("Stopped", TorrentDisplayEntry::State::Stopped);

    return vbox({// columns
                 hbox({vbox(idElems), vbox({separator()}), vbox(stateElems), vbox({separator()}),
                       vbox(nameElems) | xflex, vbox({separator()}), vbox(sizeElems),
                       vbox({separator()}), vbox(progressElems), vbox({separator()}),
                       vbox(downElems), vbox({separator()}), vbox(upElems), vbox({separator()}),
                       vbox(seederElems), vbox({separator()}), vbox(leecherElems),
                       vbox({separator()}), vbox(etaElems) | xflex}) |
                     flex,
                 hbox({text(m_feedback.m_msg) | color(feedBackColor(m_feedback))}),
                 separator() | color(Color::GreenLight),
                 hbox({m_terminal_input->Render()}) | color(Color::GreenLight),
                 separator() | color(Color::GreenLight)}) |
           bold | flex | color(Color::Blue);
}

} // namespace fractals::app