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
    std::shared_ptr<std::mutex> mu = make_shared<std::mutex>();
    std::unique_lock<std::mutex> lock(*mu.get());

    auto torr = Torrent::read_torrent("/home/ferdinand/dev/Fractals/examples/ubuntu.torrent");
    auto torr_ptr = std::make_shared<Torrent>(torr);

    boost::asio::io_context io;
    auto bt = BitTorrent(torr_ptr,io);

    bt.run();
}

// int main() {
//     std::ifstream fs;
//     fs.open("/home/ferdinand/dev/Fractals/examples/ubuntu.torrent",std::ios::binary);
//     stringstream ss;
//     ss << fs.rdbuf();
//     auto v = bencode::decode<bencode::bdata>(ss);
//     const bdata bd = v.value();
//     neither::Either<std::string,MetaInfo> emi = BencodeConvert::from_bdata<MetaInfo>(bd);

//     auto tr = emi.rightMap(makeTrackerRequest);
//     auto resp = tr.rightFlatMap(sendTrackerRequest);
//     if(resp.isLeft) {
//         cout << resp.leftValue << endl;
//     } else {
//         cout << resp.rightValue << endl;
//     }

//     int i = 12345670;

//     char c4 = i;
//     char c3 = i >> 8;
//     char c2 = i >> 16;
//     char c1 = i >> 24;
//     std::bitset<8> b4(c4);
//     std::bitset<8> b3(c3);
//     std::bitset<8> b2(c2);
//     std::bitset<8> b1(c1);
//     std::bitset<32> bi(i);

//     cout << b1 << b2 << b3 << b4 << endl;
//     cout << bi << endl;

//     bool f = false;
//     bool t = true;
//     cout << sizeof(f) << endl;
//     cout << sizeof(t) << endl;
//     std::bitset<8> bf(f);
//     std::bitset<8> bt(t << 7);
//     cout << bf << endl;
//     cout << bt << endl;
//     char c = 0;
//     std::bitset<8> a(c);
//     cout << a << endl;

//     boost::asio::io_service io_service;
//     //socket creation
//      tcp::socket socket(io_service);
//     //connection
//     auto peer = resp.rightValue.peers.front();
//     auto peer_ip = peer.ip;
//     auto peer_port = peer.port; 
//     cout << peer.peer_id << endl;
//     // 155.94.241.194:51413
//      socket.connect( tcp::endpoint( boost::asio::ip::address::from_string(peer_ip), peer_port ));
//     // request/message from client
//     auto info_hash = tr.rightValue.info_hash;
//     auto peer_id   = tr.rightValue.peer_id;
//     char reserved[8] = {0,0,0,0,0,0,0,0};
//     auto handshake = HandShake(19,"BitTorrent protocol",reserved,info_hash,peer_id);
//     auto handshake_bytes = handshake.to_bytes_repr();
//     cout << handshake_bytes.size() << endl;
//     cout << peer_id.size() << endl;
//     cout << info_hash.size() << endl;
//     // cout << std::string(handshake.to_bytes_repr().begin(),handshake.to_bytes_repr().end()) << endl;
//      const string msg1 = "Hello from Client!\n";
//      boost::system::error_code error;
//      boost::asio::write( socket, boost::asio::buffer(handshake.to_bytes_repr()), error );
//      if( !error ) {
//         cout << "Client sent hello message!" << endl;
//      }
//      else {
//         cout << "send failed: " << error.message() << endl;
//      }
//     // getting response from server
//     std::deque<char> buff;
//     boost::asio::streambuf receive_buffer;
//     // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//     while(buff.size() < 68) {
//         auto n = boost::asio::read(socket, receive_buffer,boost::asio::transfer_exactly(68), error);
//         if( error && error != boost::asio::error::eof ) {
//             cout << "receive failed: " << error.message() << endl;
//         }
//         else {
//             // cout << string(data) << " s" << endl;
//             string msg(boost::asio::buffers_begin(receive_buffer.data())
//                       ,boost::asio::buffers_end(receive_buffer.data()));
//             cout << "Msg size: " << msg.size() << " | " << n << endl;
//             cout << "Read: " << msg << endl;
//             std::copy(msg.begin(),msg.end(),back_inserter(buff));

//             receive_buffer.consume(n);
//         }    
//     }

//     char pstrlen = buff.front();
//     buff.pop_front();
//     auto hs = HandShake::from_bytes_repr(pstrlen, buff);
//     auto rsv = hs->m_reserved;
//     std::deque<char> vec_reserved;
//     vec_reserved.assign(rsv,rsv+8);
//     auto bitfield = bytes_to_bitfield(vec_reserved.size(), vec_reserved);
//     std::string str_bf;
//     for(auto b : bitfield) { b ? str_bf.push_back('1') : str_bf.push_back('0'); };

//     cout << "pstrlen: " << int(hs->m_pstrlen) << endl;
//     cout << "pstr: " << hs->m_pstr << endl;
//     cout << "rsv: " << str_bf << endl;
//     cout << "ih: " << bytes_to_hex(hs->m_info_hash) << endl;
//     cout << "p: " << bytes_to_hex(hs->m_peer_id) << endl;


//     boost::asio::write(socket,boost::asio::buffer(Interested().to_bytes_repr()),error);

//     auto im = read_message(socket);

//     auto m_mt = im->get_messageType();
//     cout << messageType_to_string(m_mt.value()) << endl;
// };