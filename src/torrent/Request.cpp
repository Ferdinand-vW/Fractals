#include <cstring>
#include <fstream>
#include <openssl/sha.h>
#include <curl/curl.h>

#include "bencode/encode.h"
#include "torrent/Request.h"

Request Request::make_request(MetaInfo mi) {
    bdict info_dict = BencodeConvert::to_bdict(mi.info);
    auto encoded = bencode::encode(info_dict);
    const char * bencode_bytes = encoded.c_str();

    unsigned char info_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)bencode_bytes, encoded.length(), info_hash);

    CURL *curl = curl_easy_init();
    char *output = curl_easy_escape(curl, (const char *)info_hash, SHA_DIGEST_LENGTH);

    if(output) {
        cout << "Sha1: ";
        for(int i = 0; i < SHA_DIGEST_LENGTH;i++) {
            printf("%02x",info_hash[i]);
        }
        cout << endl;
        printf("Encoded: %s\n", output);
        curl_free(output);
    }

    std::string peer_id = "hello";
    int port = 68812;
    int uploaded = 0;
    int downloaded = 0;
    int left = 0;
    int compact = 1;

    return Request { string(info_hash,info_hash+20)
                            , peer_id, port
                            , uploaded, downloaded
                            , left, compact
                            };
}