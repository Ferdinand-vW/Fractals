#include "torrent/Torrent.h"
#include "network/p2p/BitTorrent.h"

#include <boost/move/utility_core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/formatting_ostream.hpp>

using namespace boost::asio;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(logger, boost::log::sources::logger_mt)

int main() {
    //get current time and set as seed for rand
    srand ( time(NULL) );

    //sets attributes such as timestamp, thread id
    boost::log::add_common_attributes();

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

    auto& lg = logger::get();
    BOOST_LOG(lg) << "reaaly2";

    std::shared_ptr<std::mutex> mu = make_shared<std::mutex>();
    std::unique_lock<std::mutex> lock(*mu.get());

    auto torr = Torrent::read_torrent("/home/ferdinand/dev/Fractals/examples/[SubsPlease] Full Dive - 04 (720p) [DA12D376].mkv.torrent");
    auto torr_ptr = std::make_shared<Torrent>(torr);

    boost::asio::io_context io;
    auto bt = BitTorrent(torr_ptr,io);

    bt.run();
}
