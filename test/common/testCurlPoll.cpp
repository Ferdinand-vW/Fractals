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
    ASSERT_TRUE(result.getData().size() > 0);
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
    ASSERT_TRUE(result.getData().size() <= 0);
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
    ASSERT_TRUE(result.getData().size() <= 0);
}

} // namespace fractals::common