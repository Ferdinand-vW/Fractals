#pragma once

#include <deque>
#include <string>
#include <vector>

#include <openssl/sha.h>


namespace fractals::common {

    /**
    SHA1 encoding of byte data
    @param bytes input string where each individual character is considered a byte
    @return a vectorized sequence of bytes returned by the SHA1 encoder
    */
    template <typename Container>
    std::vector<char> sha1_encode(const Container& bytes)
    {
         //use openssl lib to generate a SHA1 hash
        unsigned char info_hash[SHA_DIGEST_LENGTH];
        SHA1((unsigned char*)(&bytes[0]), bytes.size(), info_hash);

        return std::vector<char>(info_hash,info_hash + SHA_DIGEST_LENGTH);
    }

    /**
    URL encoding of byte data
    @param bytes vectorised input sequence of bytes
    @return string type representing the URL encoding of @bytes
    */
    std::string url_encode (const std::vector<char> &bytes);

    std::string ascii_decode(const std::vector<char> &bytes);
    std::string ascii_decode(const std::deque<char> &bytes);

}