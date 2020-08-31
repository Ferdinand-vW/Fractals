#include <fstream>
#include <map>
#include "deps/bencode.h"
#include "MetaInfo.h"


MetaInfo deserialize(std::istream stream) {
  auto data = bencode::decode(stream);
  return parseMetaInfo(data);
}

MetaInfo parseMetaInfo(bencode::data content) {

  auto mi = boost::get<std::map<std::string, bencode::data>>(content);

  auto info = parseInfoDict(mi["info"]);

}

InfoDict parseInfoDict (bencode::data data) {
  auto id = boost::get<std::map<std::string,bencode::data>>(data);

  auto piecelength = boost::get<int32_t>(id["pieceLength"]);
  auto pieces = boost::get<std::string>(id["pieces"]);
  
  auto pub_it = id.find("publish"); 
  auto publish = pub_it != id.end() ? std::optional(pub_it->second) : std::nullopt;

  auto dir_it = id.find("directory");
  auto directory = dir_it != id.end() ? (dir_it->first) : "." ;

  std::cout << directory;

  return;
   
}
