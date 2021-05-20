#include "torrent/Torrent.h"
#include "network/p2p/BitTorrent.h"
#include "common/logger.h"
#include "persist/storage.h"
#include "sqlite_orm/sqlite_orm.h"

using namespace boost::asio;


int main() {
    Storage storage = init_storage("torrents.db");
    storage.sync_schema();

    auto torr = TorrentModel{56,"name","meta","write"};
    auto torr_id = storage.insert(torr);
    std::cout << torr_id << std::endl;

    // //get current time and set as seed for rand
    // srand ( time(NULL) );

    // //sets attributes such as timestamp, thread id
    // boost::log::add_common_attributes();

    // //set log file as logging destination
    // boost::log::add_file_log(
    //     boost::log::keywords::file_name = "log.txt",
    //     //this second parameter is necessary to force the log file
    //     //to be named 'log.txt'. It is a known bug in boost1.74
    //     // https://github.com/boostorg/log/issues/104
    //     boost::log::keywords::target_file_name = "log.txt",
    //     boost::log::keywords::auto_flush = true,
    //     boost::log::keywords::format = "[%ThreadID%][%TimeStamp%] %Message%"
    // );

    // std::shared_ptr<std::mutex> mu = make_shared<std::mutex>();
    // std::unique_lock<std::mutex> lock(*mu.get());

    // auto torr = Torrent::read_torrent("/home/ferdinand/dev/Fractals/examples/shamanking.torrent");
    // auto torr_ptr = std::make_shared<Torrent>(torr);

    // boost::asio::io_context io;
    // auto bt = BitTorrent(torr_ptr,io);

    // try {
    //     bt.run();
    // }
    // catch(std::exception e) {
    //     std::cout << e.what() << std::endl;
    // }
}
