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
    CurlResponse(std::vector<char> &&data, CURLcode statusCode) : data(std::move(data)), easyStatusCode(statusCode)
    {
    }

    CurlResponse(CURLcode statusCode) : easyStatusCode(statusCode)
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

  private:
    std::vector<char> data;
    std::optional<CURLMcode> multiStatusCode;
    std::optional<CURLcode> easyStatusCode;
};

class CurlPoll
{
  public:
    CurlPoll();
    ~CurlPoll();

    void add(const std::string &request, std::chrono::milliseconds timeout);

    CurlResponse poll();
    bool hasRunningTransfers();

  private:
    void cleanupRequest(CURL *handle);

    CURLM *curl{nullptr};

    int runningHandles{0};

    std::unordered_map<CURL *, std::vector<char>> buffers;
};
} // namespace fractals::common