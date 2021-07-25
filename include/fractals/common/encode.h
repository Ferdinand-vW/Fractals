#pragma once

#include <string>
#include <vector>

namespace fractals::common {

    /**
    SHA1 encoding of byte data
    @param bytes input string where each individual character is considered a byte
    @return a vectorized sequence of bytes returned by the SHA1 encoder
    */
    std::vector<char> sha1_encode(std::string bytes);

    /**
    URL encoding of byte data
    @param bytes vectorised input sequence of bytes
    @return string type representing the URL encoding of @bytes
    */
    std::string url_encode (const std::vector<char> &bytes);

}