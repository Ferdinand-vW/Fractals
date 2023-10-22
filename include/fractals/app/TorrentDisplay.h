#pragma once

#include <functional>
#include <map>
#include <string>

#include <ftxui/component/component_base.hpp> // for ComponentBase
#include <ftxui/dom/elements.hpp>

#include "fractals/app/Feedback.h"
#include "fractals/app/TorrentDisplayEntry.h"

namespace neither
{
template <class L, class R> struct Either;
}
namespace ftxui
{
class StringRef;
}

namespace fractals::app
{

/**
UI component for displaying BitTorrent connections
*/
class TorrentDisplayBase : public ftxui::ComponentBase
{
  private:
    ftxui::Component terminalInput;
    Feedback feedback;
    const std::unordered_map<uint64_t, TorrentDisplayEntry> &torrents;

    void unSetFeedback();

  public:
    /**
    Functions to be called when certain user interactions happen
    */
    std::function<void(std::string)> onAdd;
    std::function<void(uint64_t)> onRemove;
    std::function<void(uint64_t)> onStop;
    std::function<void(uint64_t)> onResume;

    TorrentDisplayBase(ftxui::Component terminalInput,
                       const std::unordered_map<uint64_t, TorrentDisplayEntry> &torrents);
    ~TorrentDisplayBase() override = default;

    /**
    Accept instance of TorrentDisplayBase from Component
    */
    static TorrentDisplayBase *From(ftxui::Component component);

    /**
    ComponentBase implementation
    */
    ftxui::Element Render() override;
    bool parseCommand(ftxui::StringRef ws);

    void setFeedBack(const Feedback &feedback);
};

/**
Function to be called when constructing instance of TorrentDisplayBase
*/
ftxui::Component TorrentDisplay(ftxui::Component terminalInput,
                                const std::unordered_map<uint64_t, TorrentDisplayEntry> &torrents);
} // namespace fractals::app