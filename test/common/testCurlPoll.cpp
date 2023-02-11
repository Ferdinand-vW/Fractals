#include <chrono>
#include <fractals/common/CurlPoll.h>

#include <gtest/gtest.h>
#include <thread>

namespace fractals::common
{

TEST(CURLPOLL, live_test)
{
    fractals::common::CurlPoll curl;

    curl.add("google.com", std::chrono::milliseconds{100000});

    auto result = CurlResponse();
    do
    {
        result = curl.poll();
    } while (curl.hasRunningTransfers());

    ASSERT_TRUE(result);
}

TEST(CURLPOLL, live_test_timeout)
{
    fractals::common::CurlPoll curl;

    curl.add("google.com", std::chrono::milliseconds{1});

    auto result = CurlResponse();
    do
    {
        result = curl.poll();
    } while (curl.hasRunningTransfers());

    ASSERT_FALSE(result);
}

TEST(CURLPOLL, bad_host)
{
    fractals::common::CurlPoll curl;

    curl.add("gooasfaawewgle.com", std::chrono::milliseconds{10000});

    auto result = CurlResponse();
    do
    {
        result = curl.poll();
    } while (curl.hasRunningTransfers());

    ASSERT_FALSE(result);
}

} // namespace fractals::common