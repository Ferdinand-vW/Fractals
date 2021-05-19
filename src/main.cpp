#include "torrent/Torrent.h"
#include "network/p2p/BitTorrent.h"
#include "common/logger.h"
#include "persist/init_db.h"
#include "sqlite_orm/sqlite_orm.h"

using namespace boost::asio;

struct TorrentF {
    int id;
    std::string name;
    std::string meta_info_path;
    std::string write_path;
};

struct PieceF {
    int id;
    int torrent_id;
    int piece;
};

struct Announce {
    int id;
    int torrent_id;
    time_t datetime;
};

struct AnnouncePeer {
    int id;
    int announce_id;
    std::string ip;
    int port;
};

int main() {
    using namespace sqlite_orm;
    auto storage = make_storage("torrents.db",
                                make_table("torrent",
                                           make_column("id", &TorrentF::id, primary_key()),
                                           make_column("name", &TorrentF::name),
                                           make_column("meta_info_path", &TorrentF::meta_info_path),
                                           make_column("write_path", &TorrentF::write_path)),
                                make_table("torrent_piece",
                                           make_column("id", &PieceF::id, primary_key()),
                                           make_column("torrent_id", &PieceF::torrent_id),
                                           make_column("piece", &PieceF::piece),
                                           foreign_key(&PieceF::torrent_id).references(&TorrentF::id)),
                                make_table("torrent_announce",
                                          make_column("id",&Announce::id,primary_key()),
                                          make_column("torrent_id",&Announce::torrent_id),
                                          make_column("datetime",&Announce::datetime),
                                          foreign_key(&Announce::torrent_id).references(&TorrentF::id)),
                                make_table("announce_peer",
                                         make_column("id",&AnnouncePeer::id,primary_key()),
                                         make_column("announce_id",&AnnouncePeer::announce_id),
                                         make_column("ip",&AnnouncePeer::ip),
                                         make_column("port",&AnnouncePeer::port),
                                         foreign_key(&AnnouncePeer::announce_id).references(&Announce::id)));
    storage.sync_schema();

    auto torr = TorrentF{56,"name","meta","write"};
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
