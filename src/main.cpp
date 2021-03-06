#include <cstdlib>       // std::abort
#include <exception>     // std::set_terminate
#include <filesystem>
#include <fstream>
#include <signal.h>

#include <boost/filesystem.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/stacktrace.hpp>

#include "fractals/app/TorrentController.h"
#include "fractals/persist/storage.h"

// useful for debugging
void my_terminate_handler() {
    try {
        std::ofstream ofs("err.txt");
        ofs << boost::stacktrace::stacktrace();
        ofs.close();
    } catch (...) {}
    std::abort();
}

void my_segfault_handler(int /* sig */) {
    try {
        std::ofstream ofs("err.txt");
        ofs << boost::stacktrace::stacktrace();
        ofs.close();
    } catch (...) {}
    std::abort();
}

int main(int /* argc */, const char* /* argv */[]) {
    boost::filesystem::path::imbue(std::locale("C"));
    std::set_terminate(&my_terminate_handler);
    signal(SIGSEGV,my_segfault_handler);
    //init and connect to local db
    fractals::persist::Storage storage;
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
        boost::log::keywords::format = "[%ThreadID%][%TimeStamp%] %Message%",
        boost::log::keywords::enable_final_rotation = false
    );

    //enables ThreadID and TimeStamp to appear in log
    boost::log::add_common_attributes();

    boost::asio::io_context io;
    fractals::app::TorrentController tc(io,storage);
    
    tc.run();
    boost::log::core::get()->remove_all_sinks();

    return 0;
}
