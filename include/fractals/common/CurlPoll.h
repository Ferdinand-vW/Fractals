#pragma once

#include <cassert>
#include <chrono>
#include <cstdint>
#include <curl/curl.h>

#include <curl/multi.h>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace fractals::common
{
class CurlResponse
{
  public:
    CurlResponse()
    {
    }
    CurlResponse(uint64_t requestId, std::vector<char> &&data, CURLcode statusCode)
        : requestId(requestId), data(std::move(data)), easyStatusCode(statusCode)
    {
    }

    CurlResponse(uint64_t requestId, CURLcode statusCode) : requestId(requestId), easyStatusCode(statusCode)
    {
    }
    CurlResponse(CURLMcode statusCode) : multiStatusCode(statusCode)
    {
    }

    operator bool() const
    {
        assert(!(multiStatusCode && easyStatusCode));

        if (multiStatusCode)
        {
            return multiStatusCode.value() == CURLMcode::CURLM_OK;
        }
        else if (easyStatusCode)
        {
            return easyStatusCode.value() == CURLcode::CURLE_OK;
        }

        return false;
    }

    const std::vector<char> &getData() const
    {
        return data;
    }

    std::vector<char> &&extractData()
    {
        return std::move(data);
    }

    const uint64_t getRequestId() const
    {
        return requestId;
    }

    std::string getError() const
    {
        if (multiStatusCode && multiStatusCode != CURLMcode::CURLM_OK)
        {
            return curl_multi_strerror(multiStatusCode.value());
        }

        if (easyStatusCode && easyStatusCode != CURLcode::CURLE_OK)
        {
            return curl_easy_strerror(easyStatusCode.value());
        }

        return "";
    }

  private:
    std::vector<char> data;
    uint64_t requestId;
    std::optional<CURLMcode> multiStatusCode;
    std::optional<CURLcode> easyStatusCode;
};

class CurlPoll
{
  public:
    CurlPoll();
    ~CurlPoll();

    uint64_t add(const std::string &request, std::chrono::milliseconds timeout);

    CurlResponse poll();
    bool hasRunningTransfers();

  private:
    void cleanupRequest(CURL *handle);

    CURLM *curl{nullptr};

    int runningHandles{0};
    uint64_t requestIdGenerator{0};

    std::unordered_map<CURL *, std::pair<uint64_t, std::vector<char>>> buffers;
};
} // namespace fractals::common