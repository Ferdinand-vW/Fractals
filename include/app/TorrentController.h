#pragma once

#include "ftxui/component/screen_interactive.hpp"
#include <ftxui/component/component.hpp>
#include "app/ScreenTicker.h"
#include <boost/asio/detail/thread_group.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/log/sources/logger.hpp>
#include <neither/either.hpp>

//forward declarations
class BitTorrent;
class Storage;
class Torrent;

typedef std::string TorrentName;

class TorrentController {
    public:
        TorrentController(boost::asio::io_context &m_io,Storage &st);
        void run();
        void exit();

    private:
        boost::asio::io_context &m_io;
        Storage &m_storage;
        
        std::mutex m_mutex;
        boost::log::sources::logger_mt &m_lg;

        int m_torrent_counter = 0;
        int m_thread_count = 4;
        boost::asio::detail::thread_group m_threads;
        using work_guard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
        std::optional<work_guard> m_work;

        //unique id for each torrent, displayed under # column
        std::map<int,std::shared_ptr<BitTorrent>> m_torrents;
        
        std::optional<ftxui::Component> m_display;
        std::optional<ftxui::Component> m_terminal;
        std::wstring m_terminal_input;

        ftxui::ScreenInteractive m_screen;
        ScreenTicker m_ticker;


        //callback functions to be passed to view
        neither::Either<std::string, TorrentName> on_add(std::string filepath);
        void add_torrent(std::shared_ptr<BitTorrent> bt);
        neither::Either<std::string, TorrentName> on_remove(int torr_id);
        neither::Either<std::string, TorrentName> on_stop(int torr_id);
        neither::Either<std::string, TorrentName> on_resume(int torr_id);
        int list_torrent(std::shared_ptr<BitTorrent> torrent);
        void start_torrent(int torr_id);
        void stop_torrents();
        void on_exit();

        void runUI();

        std::shared_ptr<BitTorrent> to_bit_torrent(std::shared_ptr<Torrent> torr);
};