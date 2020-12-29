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
#include <neither/neither.hpp>
#include "torrent/BencodeConvert.h"
#include "utils.h"
// #include "bencode/error.h"

int main() {
    std::ifstream fs;
    fs.open("/home/ferdinand/dev/Fractals/examples/myTorrent2.torrent",std::ios::binary);
    stringstream ss;
    ss << fs.rdbuf();
    auto v = bencode::decode<bencode::bdata>(ss);
    auto bdict = v.value().get_bdict().value();
    for (auto kvp : bdict.key_values()) {
        bencode::bstring k = kvp.first;
        cout << "Key: " << k << " " << kvp.second.display_type() << endl;
    }

    auto eth = BencodeConvert::from_bencode(bdict);
    cout << eth.rightValue.to_string() << endl;

    auto ann = bdict.at("announce");
    cout << "announce: " << ann << endl;
    auto ann_list = bdict.at("announce-list");
    // cout << *ann_list << endl;

    auto comment = bdict.at("comment");
    cout << "comment: " << comment << endl;

    auto created_by = bdict.at("created by");
    cout << "created by: " << created_by << endl;
    auto creation_date = bdict.at("creation date");
    cout << "creation date: " << creation_date << endl;
    auto encoding = bdict.at("encoding");
    cout << "encoding: " << encoding << endl;

    auto info = bdict.at("info").get_bdict().value();
    for (auto kvp : info.key_values()) {
        bencode::bstring k = kvp.first;
        cout << k << " " << kvp.second.display_type() << endl;
    }

    auto name = info.at("name");
    cout << "name: " << name << endl;
    auto piece_length = info.at("piece length");
    cout << "piece length: " << piece_length << endl;

    auto files = info.at("files");
    for (auto item : files.get_blist()->value()) {
        bencode::bdict k = item.get_bdict().value();
        cout << k << endl;
    }
};
