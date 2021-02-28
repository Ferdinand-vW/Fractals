#include <iostream>
#include <algorithm>
#include <bitset>
#include <c++/7/bits/c++config.h>
#include <iterator>
#include <fstream>
// #include "bencoding/bencoding.h"
// #include "bencoding/PrettyPrinter.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <curl/curl.h>
#include <bencode/bencode.h>
#include <neither/neither.hpp>

#include "network/p2p/Message.h"
#include "torrent/MetaInfo.h"
#include "torrent/BencodeConvert.h"
#include "network/http/Tracker.h"


#include "common/utils.h"
// #include "bencode/error.h"

int main() {
    std::ifstream fs;
    fs.open("/home/ferdinand/dev/Fractals/examples/ubuntu.torrent",std::ios::binary);
    stringstream ss;
    ss << fs.rdbuf();
    auto v = bencode::decode<bencode::bdata>(ss);
    const bdata bd = v.value();
    neither::Either<std::string,MetaInfo> emi = BencodeConvert::from_bdata<MetaInfo>(bd);


    int i = 12345670;

    char c4 = i;
    char c3 = i >> 8;
    char c2 = i >> 16;
    char c1 = i >> 24;
    std::bitset<8> b4(c4);
    std::bitset<8> b3(c3);
    std::bitset<8> b2(c2);
    std::bitset<8> b1(c1);
    std::bitset<32> bi(i);

    cout << b1 << b2 << b3 << b4 << endl;
    cout << bi << endl;

    bool f = false;
    bool t = true;
    cout << sizeof(f) << endl;
    cout << sizeof(t) << endl;
    std::bitset<8> bf(f);
    std::bitset<8> bt(t << 7);
    cout << bf << endl;
    cout << bt << endl;
    char c = 0;
    std::bitset<8> a(c);
    cout << a << endl;

    
    // const TrackerRequest tr = makeTrackerRequest(emi.rightValue).rightValue;
    // for(int i = 0;i<20;i++) {
    //     printf("%02x",tr.info_hash[i]);
    // }

    // auto resp = sendTrackerRequest(tr);
    // if(resp.isLeft) { cout << resp.leftValue << endl; }
    
    // cout << resp.isLeft << endl;
};
