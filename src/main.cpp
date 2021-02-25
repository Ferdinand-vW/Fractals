#include <iostream>
#include <algorithm>
#include <c++/7/bits/c++config.h>
#include <iterator>
#include <fstream>
// #include "bencoding/bencoding.h"
// #include "bencoding/PrettyPrinter.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <curl/curl.h>
#include <bencode/bencode.h>
#include <neither/neither.hpp>

#include "torrent/MetaInfo.h"
#include "torrent/BencodeConvert.h"
#include "network/http/Tracker.h"


#include "common/utils.h"
// #include "bencode/error.h"

size_t write_data(void *contents,std::size_t size,std::size_t nmemb, void *userp) {
    std::string * uptr = (std::string*)userp;
    uptr->append((char*)contents,size * nmemb);
    return size * nmemb;
}

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  const char *text;
  (void)handle; /* prevent compiler warning */
  (void)userp;
 
  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  cout << text << endl;
 
  return 0;
}

int main() {
    std::ifstream fs;
    fs.open("/home/ferdinand/dev/Fractals/examples/ubuntu.torrent",std::ios::binary);
    stringstream ss;
    ss << fs.rdbuf();
    auto v = bencode::decode<bencode::bdata>(ss);
    const bdata bd = v.value();
    neither::Either<std::string,MetaInfo> emi = BencodeConvert::from_bdata<MetaInfo>(bd);
    const TrackerRequest tr = makeTrackerRequest(emi.rightValue).rightValue;
    for(int i = 0;i<20;i++) {
        printf("%02x",tr.info_hash[i]);
    }
    cout << endl;
    CURLcode res;
    CURL *curl = curl_easy_init();
    string readBuffer;
    string param = "https://torrent.ubuntu.com/announce?info_hash="+tr.url_info_hash+"&peer_id=abcdefghijklmnopqrst&port=6882"
                                          +"&uploaded=0&downloaded=0&left=1484680095&compact=0&no_peer_id=0";
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, param.c_str());
        /* example.com is redirected, so we tell libcurl to follow redirection */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
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

    cout << readBuffer;


};
