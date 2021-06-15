#pragma once

#include "ftxui/component/screen_interactive.hpp"
#include "neither/either.hpp"
#include "network/p2p/BitTorrent.h"
#include "persist/storage.h"
#include "torrent/Torrent.h"
#include "app/TorrentDisplay.h"
#include "app/TerminalInput.h"
#include <boost/asio/io_context.hpp>

class TorrentController {
    public:
        TorrentController(boost::asio::io_context &m_io,Storage &st);
        void run();

    private:
        boost::asio::io_context &m_io;
        Storage &m_storage;
        ftxui::ScreenInteractive m_screen;

        //unique id for each torrent, displayed under # column
        std::map<int,std::shared_ptr<Torrent>> torrents;
        std::map<int,std::shared_ptr<BitTorrent>> active_torrents;
        
        //callback functions to be passed to view
        Either<std::string, int> on_add(std::string filepath);
        std::optional<std::string> on_remove(int torr_id);
        std::optional<std::string> on_stop(int torr_id);
        std::optional<std::string> on_resume(int torr_id);
        void on_exit();

        void runUI();
};