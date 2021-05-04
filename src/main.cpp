#include "torrent/Torrent.h"
#include "network/p2p/BitTorrent.h"

using namespace boost::asio;
using std::string;

int main() {
    srand ( time(NULL) );

    std::shared_ptr<std::mutex> mu = make_shared<std::mutex>();
    std::unique_lock<std::mutex> lock(*mu.get());

    auto torr = Torrent::read_torrent("/home/ferdinand/dev/Fractals/examples/[SubsPlease] Full Dive - 04 (720p) [DA12D376].mkv.torrent");
    auto torr_ptr = std::make_shared<Torrent>(torr);

    boost::asio::io_context io;
    auto bt = BitTorrent(torr_ptr,io);

    bt.run();
}
