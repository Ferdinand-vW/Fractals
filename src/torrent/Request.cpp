#include <fstream>
#include <openssl/sha.h>

#include "bencode/encode.h"
#include "torrent/Request.h"

TrackerRequest TrackerRequest::make_request(MetaInfo mi) {
    bdict info_dict = BencodeConvert::to_bdict(mi.info);
    auto encoded = bencode::encode(info_dict);

    /*
    d4:infod4:name5:b.txt6:lengthi1e12:piece lengthi32768e6:pieces20:1234567890abcdefghijee
    
    should be

    34FCC6C1ACC8C8A56DE3C2EF20924043CC51685E
    */
    const char * bencode_bytes = encoded.c_str();
    // const char * bencode_bytes = "d4:name5:b.txt6:lengthi1e12:piece lengthi32768e6:pieces20:1234567890abcdefghije";
    std::ofstream fs;
    fs.open("test.txt",ios::binary);
    fs << ("d4:info"s + encoded +  "e");
    fs.close();
    unsigned char info_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)bencode_bytes, strlen(bencode_bytes), info_hash);
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        printf("%02x",info_hash[i]);
    }
    cout << endl;

    std::string peer_id = "hello";
    int port = 68812;
    int uploaded = 0;
    int downloaded = 0;
    int left = 0;
    int compact = 1;

    return TrackerRequest { string(info_hash,info_hash+20)
                            , peer_id, port
                            , uploaded, downloaded
                            , left, compact
                            };
}