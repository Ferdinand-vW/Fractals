#include <iostream>
#include <algorithm>
#include <iterator>
#include <fstream>
// #include "bencoding/bencoding.h"
// #include "bencoding/PrettyPrinter.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <bencode/bencode.h>

#include "torrent/MetaInfo.h"
#include "torrent/BencodeConvert.h"
#include "torrent/Request.h"
#include <neither/neither.hpp>

#include "utils.h"
// #include "bencode/error.h"

int main() {
    std::ifstream fs;
    fs.open("/home/ferdinand/dev/Fractals/examples/alice.torrent",std::ios::binary);
    stringstream ss;
    ss << fs.rdbuf();
    auto v = bencode::decode<bencode::bdata>(ss);
    const bdata bd = v.value();
    neither::Either<std::string,MetaInfo> emi = BencodeConvert::from_bdata<MetaInfo>(bd);

    TrackerRequest tr = TrackerRequest::make_request(emi.rightValue);

stringstream ss2("d4:name5:b.txt6:lengthi1e12:piece lengthi32768e6:pieces20:1234567890abcdefghije");
    auto b_test = bencode::decode<bencode::bdata>(ss2);
    neither::Either<std::string,InfoDict> eid = BencodeConvert::from_bdata<InfoDict>(b_test.value());
    InfoDict info_dict = eid.rightValue;
    auto info_dict_enc = BencodeConvert::to_bdict(info_dict);
    auto info_dict_s = bencode::encode(info_dict_enc);
    cout << info_dict_s << endl;


};
