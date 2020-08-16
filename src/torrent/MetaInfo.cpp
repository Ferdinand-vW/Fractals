#include <fstream>

#include "MetaInfo.h"


MetaInfo deserialize(std::istream stream) {
  auto data = bencode::decode(stream);

  auto value = boost::get<std::map<std::string, bencode::data>>(content);

  

}
