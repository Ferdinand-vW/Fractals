#include <iostream>
#include <algorithm>
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
    const TrackerRequest tr = makeTrackerRequest(emi.rightValue).rightValue;
    for(int i = 0;i<20;i++) {
        printf("%02x",tr.info_hash[i]);
    }

    auto resp = sendTrackerRequest(tr);
    if(resp.isLeft) { cout << resp.leftValue << endl; }
    
    cout << resp.isLeft << endl;
};
