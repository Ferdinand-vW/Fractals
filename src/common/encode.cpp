#include "common/encode.h"
#include <cstring>
#include <curl/curl.h>
#include <curl/easy.h>
#include <memory>
#include <iostream>
#include <openssl/sha.h>
#include <vector>

std::vector<char> sha1_encode(std::string bytes) {
    unsigned char info_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)bytes.c_str(), bytes.length(), info_hash);

    return std::vector<char>(info_hash,info_hash + SHA_DIGEST_LENGTH);
}

std::string url_encode (const std::vector<char> &ptr) {
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