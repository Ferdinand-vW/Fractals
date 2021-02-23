#include "common/encode.h"
#include <cstring>
#include <memory>
#include <iostream>
#include <openssl/sha.h>
#include <vector>

vector<unsigned char> sha1_encode(string bytes) {
    unsigned char info_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)bytes.c_str(), bytes.length(), info_hash);

    return vector<unsigned char>(info_hash,info_hash + SHA_DIGEST_LENGTH);
}

neither::Maybe<string> url_encode (const vector<unsigned char> &ptr) {
    CURL *curl = curl_easy_init();
    const char *output = curl_easy_escape(curl, (char const *)ptr.data(), SHA_DIGEST_LENGTH);

    if(output) { curl_free(curl); return string(output); }
    else { return {}; }

}