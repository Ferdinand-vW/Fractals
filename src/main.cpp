#include <iostream>
#include <algorithm>
#include <bitset>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <deque>
#include <functional>
#include <iterator>
#include <fstream>
// #include "bencoding/bencoding.h"
// #include "bencoding/PrettyPrinter.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <boost/asio.hpp>

#include <curl/curl.h>
#include <bencode/bencode.h>
#include <neither/neither.hpp>

#include "network/p2p/Message.h"
#include "network/p2p/MessageType.h"
#include "torrent/MetaInfo.h"
#include "torrent/BencodeConvert.h"
#include "torrent/Torrent.h"
#include "network/http/Tracker.h"
#include "network/p2p/BitTorrent.h"


#include "common/utils.h"
// #include "bencode/error.h"

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;

int main() {
    srand ( time(NULL) );

    std::shared_ptr<std::mutex> mu = make_shared<std::mutex>();
    std::unique_lock<std::mutex> lock(*mu.get());

    auto torr = Torrent::read_torrent("/home/ferdinand/dev/Fractals/examples/Golden Time [LN].torrent");
    auto torr_ptr = std::make_shared<Torrent>(torr);

    boost::asio::io_context io;
    auto bt = BitTorrent(torr_ptr,io);

    bt.run();
}
