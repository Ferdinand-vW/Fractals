#include <vector>

#include <curl/curl.h>
#include <openssl/sha.h>

#include "fractals/common/encode.h"

namespace fractals::common {

    std::vector<char> sha1_encode(std::string bytes) {
        //use openssl lib to generate a SHA1 hash
        unsigned char info_hash[SHA_DIGEST_LENGTH];
        SHA1((unsigned char*)bytes.c_str(), bytes.length(), info_hash);

        return std::vector<char>(info_hash,info_hash + SHA_DIGEST_LENGTH);
    }

    std::string url_encode (const std::vector<char> &ptr) {
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

    std::string ascii_decode(const std::deque<char> &bytes)
    {
        return std::string(bytes.begin(), bytes.end());
    }

}