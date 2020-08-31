#include <iostream>
#include <iterator>
#include <fstream>
// #include "bencoding/bencoding.h"
// #include "bencoding/PrettyPrinter.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "bencoding/bencoding.h"

int main() {

    std::ifstream file;
    file.open("examples/myTorrent2.torrent");

    auto decodedData = bencoding::decode(file);
    std::string prettyString = bencoding::getPrettyRepr(decodedData);
    std::cout << prettyString;

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
    return 0;

};