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

    Request tr = Request::make_request(emi.rightValue);
};
