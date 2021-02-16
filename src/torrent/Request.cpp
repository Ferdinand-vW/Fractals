#include <cstring>
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

    d8595f3f254b8d6ec73594a54f31a21dbbe7aef8
    */
    const char * bencode_bytes = encoded.c_str();

    // const char * bencode_bytes = "d6:lengthi163783e4:name9:alice.txt12:piece lengthi16384ee";
    std::ofstream fs;
    fs.open("example.torrent",ios::binary);
    auto encoded2 = "d13:creation datei1452468725091e8:encoding5:UTF-84:info"s + encoded + "e"s;
    fs << encoded2;
    fs.close();
    unsigned char info_hash[20];
    cout << encoded2 << endl;
    cout << strlen(bencode_bytes) << endl;
    cout << encoded.length() << endl;
    SHA1((unsigned char *)bencode_bytes, encoded.length(), info_hash);

    for(int i = 0; i < 20; i++) {
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