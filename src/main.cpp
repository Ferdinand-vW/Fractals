// #include "ftxui/screen/string.hpp"
// #include "persist/data.h"
// #include "torrent/Torrent.h"
// #include "network/p2p/BitTorrent.h"
// #include "common/logger.h"
// #include "persist/storage.h"
// #include "sqlite_orm/sqlite_orm.h"

// using namespace boost::asio;



#include "app/TorrentController.h"
#include "persist/data.h"
#include <filesystem>
#include <signal.h>
#include <boost/stacktrace.hpp>
#include <cstdlib>       // std::abort
#include <exception>     // std::set_terminate
#include <iostream>      // std::cerr

// useful for debugging
void my_terminate_handler() {
    try {
        std::cout << boost::stacktrace::stacktrace();
    } catch (...) {}
    std::abort();
}

void my_segfault_handler(int sig) {
    try {
        std::cout << boost::stacktrace::stacktrace();
    } catch (...) {}
    std::abort();
}

int main(int argc, const char* argv[]) {
    std::set_terminate(&my_terminate_handler);
    signal(SIGSEGV,my_segfault_handler);
    //init and connect to local db
    Storage storage;
    storage.open_storage("torrents.db");
    storage.sync_schema();

    if(!std::filesystem::exists("./metainfo")) {
        std::filesystem::create_directory("./metainfo");
    }

    //get current time and set as seed for rand
    srand (time(NULL));

    //set log file as logging destination
    boost::log::add_file_log(
        boost::log::keywords::file_name = "log.txt",
        //this second parameter is necessary to force the log file
        //to be named 'log.txt'. It is a known bug in boost1.74
        // https://github.com/boostorg/log/issues/104
        boost::log::keywords::target_file_name = "log.txt",
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::format = "[%ThreadID%][%TimeStamp%] %Message%"
    );

    boost::asio::io_context io;
    TorrentController tc(io,storage);
    tc.run();
}
