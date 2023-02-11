#include "fractals/network/http/TrackerClient.h"
#include "fractals/network/http/Request.h"

#include <curl/curl.h>

namespace fractals::network::http
{

namespace 
{
    std::size_t writeTrackerResponseData(void *contents, std::size_t size, std::size_t nmemb, void *userp) 
    {
        std::string *uptr = reinterpret_cast<std::string*>(userp);
        char * conts      = reinterpret_cast<char*>(contents);
        uptr->append(conts,size * nmemb);
        return size * nmemb;
    }

}

    TrackerResult TrackerClient::query(const TrackerRequest &tr, std::chrono::milliseconds recvTimeout)
    {
        CURLcode res;
        curl_global_init(0);
        CURL *curl = curl_easy_init();
        std::string readBufferedQueueManager;
        std::string url = tr.toHttpGetUrl();

        if(url.substr(0,3) == "udp") 
        {
            return "udp trackers are not supported"s;
        }

        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeTrackerResponseData);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBufferedQueueManager);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, recvTimeout);
        
            res = curl_easy_perform(curl);
            if(res != CURLE_OK) { 
                std::string curl_err = curl_easy_strerror(res);
                curl_easy_cleanup(curl); // Clean up CURL pointer
                return "CURL error: " + curl_err; 
            }
        }

        curl_easy_cleanup(curl);

        std::stringstream ss(readBufferedQueueManager);
        auto mresp = bencode::decode<bencode::bdict>(ss);

        if (!mresp.has_value()) 
        { 
            return mresp.error().message();
        }

        auto resp = mresp.value();

        return TrackerResponse::decode(resp);
    }

}