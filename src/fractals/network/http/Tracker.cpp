#include <fstream>
#include <functional>
#include <netinet/in.h>
#include <curl/curl.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <algorithm>
#include <optional>

#include <boost/outcome/basic_result.hpp>
#include <bencode/bdata.h>
#include <bencode/bdict.h>
#include <bencode/bint.h>
#include <bencode/blist.h>
#include <bencode/bstring.h>
#include <bencode/error.h>
#include <bencode/decode.h>
#include <bencode/encode.h>
#include <neither/neither.hpp>

#include "fractals/app/Client.h"
#include "fractals/common/encode.h"
#include "fractals/common/maybe.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Tracker.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/Peer.h"
#include "fractals/torrent/Bencode.h"
#include "fractals/torrent/MetaInfo.h"

using namespace neither;

namespace fractals::network::http {

    namespace {
        std::size_t writeTrackerResponseData(void *contents,std::size_t size,std::size_t nmemb,void *userp) {
            std::string *uptr = reinterpret_cast<string*>(userp);
            char * conts      = reinterpret_cast<char*>(contents);
            uptr->append(conts,size * nmemb);
            return size * nmemb;
        }
    }

    TrackerClient::TrackerClient(string &&url) : mUrl(std::move(url)) {};
    TrackerClient::TrackerClient(const std::string &url) : mUrl(url) {};

    std::string TrackerClient::getUrl() const {
        return mUrl;
    }

  

    neither::Either<std::string, TrackerResponse> TrackerClient::sendRequest(const TrackerRequest &tr)
    {
        CURLcode res;
        curl_global_init(0);
        CURL *curl = curl_easy_init();
        std::string readBuffer;
        string url = tr.toHttpGetUrl();

        if(url.substr(0,3) == "udp") {
            return left ("udp trackers are not supported"s);
        }

        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeTrackerResponseData);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        
            /* Perform the request, res will get the return code */ 
            res = curl_easy_perform(curl);
            if(res != CURLE_OK) { 
                std::string curl_err = curl_easy_strerror(res);
                /* always cleanup */ 
                curl_easy_cleanup(curl);
                return left("curl_easy_perform() failed: "+curl_err); 
            }
        }

        curl_easy_cleanup(curl);

        stringstream ss(readBuffer);
        auto mresp = bencode::decode<bencode::bdict>(ss);

        if (!mresp.has_value()) { return neither::left(mresp.error().message()); }

        auto resp = mresp.value();

        return TrackerResponse::decode(resp);
    }

    std::ostream &operator<<(ostream &os, const TrackerClient &t) {
        os << "TrackerClient(" + t.getUrl() + ")";
        return os;
    }
}
