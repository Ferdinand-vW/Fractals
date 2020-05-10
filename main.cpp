#include <iostream>
#include <iterator>
#include <fstream>
#include "deps/bencode.h"

int main() {

    std::ifstream file;
    file.open("examples/myTorrent.torrent");

    bencode::data content = bencode::decode(file);
    auto value = boost::get<std::map<std::string, bencode::data>>(content);

    std::vector<std::string> keys;

    auto begin {value.begin()};
    auto end {value.end()};

    for (auto p {begin}; p != end; p++) { keys.push_back((*p).first);}

    std::ofstream outFile("examples/out.txt");
    std::ostream_iterator<std::string> output_iter(outFile,"\n");

    auto url = boost::get<bencode::string>(value["anounce"]);
    std::cout << url;

    auto announceList = boost::get<std::vector<bencode::data>>(value["announce-list"]);
    std::transform(announceList.begin(),announceList.end(),announceList.begin(),[](bencode::data dInner)
        {
            auto innerVec = boost::get<std::vector<bencode::data>>(dInner);
            auto innerVec2 = std::transform(innerVec.begin(),innerVec.end(),innerVec.begin(),[](bencode::data dVal) 
            {
                auto val = boost::get<std::string>(dVal);
                return val;
            });
        });

    std::cout << announceList;

    auto comment = boost::get<bencode::string>(value["comment"]);
    std::cout << comment;

    auto creationDate = boost::get<long>(value["created by"]);
    std::cout << creationDate;

    auto createdBy = boost::get<std::string>(value["created by"]);
    std::cout << createdBy;

    auto encoding = boost::get<std::string>(value["encoding"]);
    std::cout << encoding;



    std::copy(keys.begin(), keys.end(), output_iter);
    return 0;

};