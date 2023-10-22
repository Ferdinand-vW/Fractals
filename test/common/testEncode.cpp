#include <fractals/common/encode.h>
#include <fractals/common/utils.h>

#include <gtest/gtest.h>
#include <vector>

namespace fractals::common
{

TEST(ENCODE, sha1_encode)
{
    std::string inp("!teststringº");

    auto enc = sha1_encode<20>(inp);

    std::string outp("1b74878ef7e38624fda0b55d9f33be10861bd7c4");

    ASSERT_EQ(outp, bytesToHex<20>(enc));
}

TEST(ENCODE, url_encode)
{
    std::string inp("teststring");

    auto enc = sha1_encode<20>(inp);
    auto urlenc = urlEncode<20>(enc);

    std::string outp("%B8G%3B%86%D4%C2%07%2C%A9%B0%8B%D2%8E7%3E%82S%E8e%C4");

    ASSERT_EQ(outp, urlenc);
}

} // namespace fractals::common