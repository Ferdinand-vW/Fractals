#include "fractals/common/CurlPoll.h"

#include <chrono>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>
#include <iostream>
#include <iterator>
#include <spdlog/spdlog.h>

namespace fractals::common
{
namespace
{
size_t read(void *received, size_t size, size_t nmemb, void *clientp)
{
    std::vector<char> &buf = *reinterpret_cast<std::vector<char> *>(clientp);
    char *data = reinterpret_cast<char *>(received);

    std::vector<char> temp(data, data + size * nmemb);

    buf.insert(buf.end(), temp.begin(), temp.end());
    return size * nmemb;
}
} // namespace

CurlPoll::CurlPoll()
{
    curl_global_init(0);

    curl = curl_multi_init();
}

CurlPoll::~CurlPoll()
{
    curl_multi_cleanup(curl);
}

uint64_t CurlPoll::add(const std::string &request, std::chrono::milliseconds timeout)
{
    CURL *handle = curl_easy_init();
    uint64_t requestId = requestIdGenerator++;

    buffers.emplace(handle, std::make_pair(requestId, std::vector<char>()));

    curl_easy_setopt(handle, CURLOPT_URL, request.c_str());
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, read);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffers[handle].second);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0L);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);

    curl_multi_add_handle(curl, handle);

    return requestId;
}

void CurlPoll::cleanupRequest(CURL *handle)
{
    buffers.erase(handle);
    curl_multi_remove_handle(curl, handle);
    curl_easy_cleanup(handle);
}

CurlResponse CurlPoll::poll()
{
    curl_multi_perform(curl, &runningHandles);

    if (runningHandles)
    {
        int numReady{0};
        CURLMcode mc = curl_multi_poll(curl, NULL, 0, 100, &numReady);

        if (mc != CURLM_OK)
        {
            spdlog::error("curl_multi_poll() failed, code %d\n", mc);
            return CurlResponse(mc);
        }
    }

    int msgsLeft;
    const auto *msg = curl_multi_info_read(curl, &msgsLeft);
    if (msg)
    {
        auto &&[reqId, buf] = std::move(buffers[msg->easy_handle]);
        const auto resp = CurlResponse{reqId, std::move(buf), msg->data.result};
        cleanupRequest(msg->easy_handle);
        return resp;
    }

    return {};
}

bool CurlPoll::hasRunningTransfers()
{
    return runningHandles;
}
} // namespace fractals::common