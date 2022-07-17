#include <fractals/common/encode.h>
#include <fractals/common/utils.h>

#include <vector>
#include <gtest/gtest.h>

TEST(ENCODE, sha1_encode)
{
    std::string inp("teststring");

    auto enc = fractals::common::sha1_encode(inp);

    std::string outp("b8473b86d4c2072ca9b08bd28e373e8253e865c4");

    ASSERT_EQ(outp, fractals::common::bytes_to_hex(enc));
}

TEST(ENCODE, url_encode)
{
    std::string inp("teststring");

    auto enc = fractals::common::sha1_encode(inp);
    auto urlenc = fractals::common::url_encode(enc);

    std::string outp("%B8G%3B%86%D4%C2%07%2C%A9%B0%8B%D2%8E7%3E%82S%E8e%C4");

    ASSERT_EQ(outp, urlenc);
}