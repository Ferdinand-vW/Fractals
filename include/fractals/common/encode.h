#pragma once

#include "fractals/common/utils.h"
#include <curl/curl.h>
#include <deque>
#include <string>
#include <vector>
#include <iostream>

#include <openssl/sha.h>


namespace fractals::common {

    /**
    SHA1 encoding of byte data
    @param bytes input string where each individual character is considered a byte
    @return a vectorized sequence of bytes returned by the SHA1 encoder
    */
    template <int N, typename Container>
    std::array<char, N> sha1_encode(const Container& bytes)
    {
         //use openssl lib to generate a SHA1 hash
        unsigned char infoHash[SHA_DIGEST_LENGTH];
        SHA1((unsigned char*)(&bytes[0]), bytes.size(), infoHash);

        std::array<char, 20> res;
        std::copy(&infoHash[0], &infoHash[20], res.data());

        return res;
    }

    /**
    URL encoding of byte data
    @param bytes vectorised input sequence of bytes
    @return string type representing the URL encoding of @bytes
    */
    std::string urlEncode (const std::vector<char> &bytes);
    template <int N>
    std::string urlEncode (const std::array<char, N>& bytes)
    {
        //use curl lib to apply url encoding
        CURL *curl = curl_easy_init();
        char * p = curl_easy_escape(curl, (char const *)bytes.data(), SHA_DIGEST_LENGTH);
        std::string output(p);
        curl_free(p);
        if(curl) {
            curl_easy_cleanup(curl);
            return output; 
        }
        else { 
            curl_easy_cleanup(curl);
            return "";
        }
    }

    std::string asciiDecode(const std::vector<char> &bytes);
    std::string asciiDecode(const std::deque<char> &bytes);

}