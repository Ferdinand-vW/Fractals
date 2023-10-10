#include <fractals/common/encode.h>
#include <fractals/common/utils.h>

#include <vector>
#include <gtest/gtest.h>

TEST(ENCODE, sha1_encode)
{
    std::string inp("!teststringº");

    auto enc = fractals::common::sha1_encode<20>(inp);

    std::string outp("1b74878ef7e38624fda0b55d9f33be10861bd7c4");

    ASSERT_EQ(outp, fractals::common::bytes_to_hex<20>(enc));
}

TEST(ENCODE, url_encode)
{
    std::string inp("teststring");

    auto enc = fractals::common::sha1_encode<20>(inp);
    auto urlenc = fractals::common::url_encode<20>(enc);

    std::string outp("%B8G%3B%86%D4%C2%07%2C%A9%B0%8B%D2%8E7%3E%82S%E8e%C4");

    ASSERT_EQ(outp, urlenc);
}