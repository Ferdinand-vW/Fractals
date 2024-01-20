#include <vector>

#include <curl/curl.h>
#include <openssl/sha.h>
#include <iostream>

#include <fractals/common/encode.h>

namespace fractals::common {

    std::vector<char> sha1_encode(std::string bytes) {
        //use openssl lib to generate a SHA1 hash
        unsigned char infoHash[SHA_DIGEST_LENGTH];
        SHA1((unsigned char*)bytes.c_str(), bytes.length(), infoHash);

        return std::vector<char>(infoHash,infoHash + SHA_DIGEST_LENGTH);
    }

    std::string urlEncode (const std::vector<char> &ptr) {
        //use curl lib to apply url encoding
        CURL *curl = curl_easy_init();
        char * p = curl_easy_escape(curl, (char const *)ptr.data(), SHA_DIGEST_LENGTH);
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

    std::string asciiDecode(const std::vector<char> &bytes)
    {
        return std::string(bytes.begin(), bytes.end());
    }

    std::string asciiDecode(const std::deque<char> &bytes)
    {
        return std::string(bytes.begin(), bytes.end());
    }

}