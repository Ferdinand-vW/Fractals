#include <c++/7/bits/c++config.h>
#include <cstring>
#include <fstream>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <string>

#include "bencode/encode.h"
#include "common/utils.h"
#include "neither/either.hpp"
#include "network/TrackerRequest.h"
#include "common/encode.h"

neither::Either<std::string,TrackerRequest> makeTrackerRequest(const MetaInfo &mi) {
    bdict info_dict = BencodeConvert::to_bdict(mi.info);
    auto encoded = bencode::encode(info_dict);

    ofstream file;
    file.open("example.txt");
    file << encoded;
    file.close();

    auto info_hash     = sha1_encode(encoded);
    auto muri_info     = url_encode(info_hash);
    auto str_info_hash = string(info_hash.begin(),info_hash.end());
    
    if(!muri_info.hasValue) { 
        return neither::left("Failed url encode of info hash: "s + str_info_hash); 
    } 
    else {
        std::string peer_id = "hello";
        int port = 68812;
        int uploaded = 0;
        int downloaded = 0;
        int left = 0;
        int compact = 1;
        const TrackerRequest request = TrackerRequest { 
                                  mi.announce
                                  , info_hash, muri_info.value
                                  , peer_id, port
                                  , uploaded, downloaded
                                  , left, compact
                                  };
        return neither::Either<string,TrackerRequest>(right(request));;
    }
}

std::size_t writeTrackerResponseData(void *contents,std::size_t size,std::size_t nmemb,void *userp) {
    std::string *uptr = reinterpret_cast<string*>(userp);
    char * conts      = reinterpret_cast<char*>(contents);
    uptr->append(conts,size * nmemb);
    return size * nmemb;
}

std::string toHttpGetUrl(const TrackerRequest &tr) {
    std::string ann_base = tr.announce + "?";
    std::string ih_prm   = "info_hash="+tr.url_info_hash;
    std::string peer_prm = "peer_id="+tr.peer_id;
    std::string port_prm = "port="+std::to_string(tr.port);
    std::string upl_prm = "uploaded="+std::to_string(tr.uploaded);
    std::string dl_prm  = "downloaded="+std::to_string(tr.downloaded);
    std::string lft_prm = "left="+std::to_string(tr.left);
    std::string cmpt_prm = "compact="+std::to_string(tr.compact);

    std::string url = tr.announce + intercalate("&", {ih_prm,peer_prm,port_prm,upl_prm,dl_prm,lft_prm,cmpt_prm});

    return url;
}

neither::Either<string, TrackerResponse> sendTrackerRequest(const TrackerRequest &tr) {
    CURLcode res;
    CURL *curl = curl_easy_init();
    string readBuffer;
    string url = toHttpGetUrl(tr);
    
    //"https://torrent.ubuntu.com/announce?info_hash="+tr.url_info_hash+"&peer_id=abcdefghijklmnopqrst&port=6882"
      //                                    +"&uploaded=0&downloaded=0&left=1484680095&compact=0&no_peer_id=0";
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        /* example.com is redirected, so we tell libcurl to follow redirection */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeTrackerResponseData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    
        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        /* Check for errors */ 
        if(res != CURLE_OK)
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));
    
        /* always cleanup */ 
        curl_easy_cleanup(curl);
    }

    const auto resultBuffer = readBuffer;

    return neither::Either<string,TrackerResponse>(neither::left(resultBuffer));
}