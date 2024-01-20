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

#include <fractals/app/Feedback.h>
#include <fractals/app/TorrentDisplay.h>
#include <fractals/app/TorrentDisplayEntry.h>
#include <fractals/common/utils.h>

using namespace ftxui;

namespace fractals::app
{

ftxui::Component TorrentDisplay(ftxui::Component terminalInput,
                                const std::unordered_map<uint64_t, TorrentDisplayEntry> &torrents)
{
    return Make<TorrentDisplayBase>(terminalInput, torrents);
}

TorrentDisplayBase::TorrentDisplayBase(
    ftxui::Component terminalInput,
    const std::unordered_map<uint64_t, TorrentDisplayEntry> &torrents)
    : terminalInput(terminalInput), torrents(torrents)
{
}

TorrentDisplayBase *TorrentDisplayBase::From(ftxui::Component component)
{
    return static_cast<TorrentDisplayBase *>(component.get());
}

void TorrentDisplayBase::unSetFeedback()
{
    feedback.msg = "";
    feedback.msgType = FeedbackType::Info;
}

void TorrentDisplayBase::setFeedBack(const Feedback& aFeedback)
{
    feedback = aFeedback;
}

// @ws contains a by user written command. Here we attempt to parse it and call the relevant
// functions.
bool TorrentDisplayBase::parseCommand(StringRef ws)
{
    std::stringstream ss(ws->data());
    std::string com;
    ss >> com;

    if (com == "exit")
    {
        feedback.msg = "Exiting...";
        feedback.msgType = FeedbackType::Warning;
        return true;
    }

    if (com == "help")
    {
        feedback.msg = "Available commands: exit, add <filepath>, stop <torrent name or id>, "
                           "resume <torrent name or id>, remove <torrent name or id>";
        feedback.msgType = FeedbackType::Info;
        return false;
    }

    if (!common::elem(com, "add", "stop", "resume", "remove"))
    {
        feedback.msg = "unknown command: " + com;
        feedback.msgType = FeedbackType::Error;

        return false;
    }

    if (!ss.rdbuf()->in_avail())
    {
        feedback.msg = "expecting exactly 1 argument but got none";
        feedback.msgType = FeedbackType::Error;

        return false;
    }

    unSetFeedback();

    // parse string identifiers with format '#<integer>'
    // the integer *should* refer to a torrent id
    // return error messages on failure
    auto parseIdent = [](auto &ident) -> neither::Either<std::string, int>
    {
        if (ident.substr(0, 1) != "#")
        {
            return neither::left<std::string>(
                "torrent identifier must start with #. Expected format is #<torrent id>.");
        }

        auto sNumm = ident.substr(1, ident.size() - 1);
        int num = -1;
        auto b = boost::conversion::try_lexical_convert<int>(sNumm, num);
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
            feedback.msg = "path does not exist: " + path;
            feedback.msgType = FeedbackType::Error;
        }
        else
        {
            onAdd(path);
        }

        return false;
    }

    if (com == "stop")
    {
        // parse and validate the second argument
        std::string ident;
        ss >> ident;
        auto argIdent = parseIdent(ident);

        if (argIdent.isLeft)
        {
            feedback.msg = argIdent.leftValue;
            feedback.msgType = FeedbackType::Error;
        }
        else
        {
            // send stop message to TorrentController
            onStop(argIdent.rightValue);
        }
    }

    if (com == "resume")
    {
        // parse and validate the second argument
        std::string ident;
        ss >> ident;
        auto argIdent = parseIdent(ident);

        if (argIdent.isLeft)
        {
            feedback.msg = argIdent.leftValue;
            feedback.msgType = FeedbackType::Error;
        }
        else
        {
            // send resume message to TorrentController
            onResume(argIdent.rightValue);
        }
    }

    if (com == "remove")
    {
        // parse and validate the second argument
        std::string ident;
        ss >> ident;
        auto argIdent = parseIdent(ident);

        if (argIdent.isLeft)
        {
            feedback.msg = argIdent.leftValue;
            feedback.msgType = FeedbackType::Error;
        }
        else
        {
            // send remove message to TorrentController
            onRemove(argIdent.rightValue);
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
        int bSize = std::floor(rem / 2);
        int aSize = std::ceil(rem / 2);
        std::string before(bSize, ' ');
        std::string after(aSize, ' ');
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
            idElems.emplace_back(cell(std::to_string(tv.getId())));
            stateElems.emplace_back(cell(s));
            nameElems.emplace_back(cell(tv.getName()));
            sizeElems.emplace_back(cell(common::ppBytes(tv.getSize())));
            progressElems.emplace_back(cell(common::ppBytes(tv.getDownloaded())));
            downElems.emplace_back(cell(common::ppBytesPerSecond(tv.getDownloadSpeed())));
            upElems.emplace_back(cell(common::ppBytesPerSecond(tv.getUploadSpeed())));
            auto connects = [](auto act, auto tot)
            {
                return std::to_string(act) + "(" + std::to_string(tot) + ")";
            };
            seederElems.emplace_back(cell(connects(tv.getConnectedSeeders(), tv.getTotalSeeders())));
            leecherElems.emplace_back(
                cell(connects(tv.getConnectedLeechers(), tv.getTotalLeechers())));
            etaElems.emplace_back(cell(common::ppTime(tv.getEta())));
        }
    };

    // set column headers
    idElems.emplace_back(colOfSize("#", 4));
    idElems.emplace_back(separator() | color(Color::GreenLight));
    stateElems.emplace_back(colOfSize("State", 10));
    stateElems.emplace_back(separator() | color(Color::GreenLight));
    nameElems.emplace_back(column("Torrent Name"));
    nameElems.emplace_back(separator() | color(Color::GreenLight));
    sizeElems.emplace_back(colOfSize("Size", 12));
    sizeElems.emplace_back(separator() | color(Color::GreenLight));
    progressElems.emplace_back(colOfSize("Progress", 16));
    progressElems.emplace_back(separator() | color(Color::GreenLight));
    downElems.emplace_back(colOfSize("Down", 12));
    downElems.emplace_back(separator() | color(Color::GreenLight));
    upElems.emplace_back(colOfSize("Up", 12));
    upElems.emplace_back(separator() | color(Color::GreenLight));
    seederElems.emplace_back(colOfSize("Seeders", 12));
    seederElems.emplace_back(separator() | color(Color::GreenLight));
    leecherElems.emplace_back(colOfSize("Leechers", 12));
    leecherElems.emplace_back(separator() | color(Color::GreenLight));
    etaElems.emplace_back(column("ETA"));
    etaElems.emplace_back(separator() | color(Color::GreenLight));

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
                 hbox({text(feedback.msg) | color(feedBackColor(feedback))}),
                 separator() | color(Color::GreenLight),
                 hbox({terminalInput->Render()}) | color(Color::GreenLight),
                 separator() | color(Color::GreenLight)}) |
           bold | flex | color(Color::Blue);
}

} // namespace fractals::app