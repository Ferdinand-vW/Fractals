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

#include "bencode/bencode.h"
// #include "bencode/error.h"

int main() {
    std::ifstream fs;
    fs.open("/home/ferdinand/dev/Fractals/examples/myTorrent2.torrent",std::ios::binary);
    stringstream ss;
    ss << fs.rdbuf();
    auto v = bencode::decode<bencode::bdata>(ss);
    auto bdict = v.value().get_bdict().value();
    auto info = bdict.value().at(bencode::bstring("info"));
    for (auto kvp : info->get_bdict()->value()) {
        bencode::bstring k = kvp.first;
        cout << k << " " << kvp.second->display_type() << endl;
    }
    // for_each(bdict.value().begin(), bdict.value().end(), [](pair<bencode::bstring,shared_ptr<bencode::bdata>> kvp) { cout << kvp.first << endl; });
    // cout << v.value() << endl;
};
    // auto decodedData = bencoding::decode(file);
    // std::string prettyString = bencoding::getPrettyRepr(decodedData);
    // std::cout << prettyString;

    // std::vector<std::string> keys;

    // auto begin {value.begin()};
    // auto end {value.end()};

    // for (auto p {begin}; p != end; p++) { keys.push_back((*p).first);}

    // std::ofstream outFile("examples/out.txt");
    // std::ostream_iterator<std::string> output_iter(outFile,"\n");

    // auto url = value["anounce"];


    // // auto announceList = boost::get<std::vector<bencode::data>>(value["announce-list"]);
    // // std::transform(announceList.begin(),announceList.end(),announceList.begin(),[](bencode::data dInner)
    // //     {
    // //         auto innerVec = boost::get<std::vector<bencode::data>>(dInner);
    // //         auto innerVec2 = std::transform(innerVec.begin(),innerVec.end(),innerVec.begin(),[](bencode::data dVal) 
    // //         {
    // //             auto val = boost::get<std::string>(dVal);
    // //             return val;
    // //         });
    // //     });

    // // std::cout << announceList;

    // std::cout << "oh";
    // auto comment = value["comment"];
    // std::cout << "test";
    // std::cout << comment;
    // std::cout << "test2";

    // auto creationDate = value["created date"];
    // std::cout << creationDate;

    // auto createdBy = value["created by"];
    // std::cout << createdBy;

    // auto encoding = value["encoding"];
    // std::cout << encoding;



    // std::copy(keys.begin(), keys.end(), output_iter);
//     return 0;

// };