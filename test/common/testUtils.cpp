#include "gmock/gmock.h"
#include <cctype>
#include <fractals/common/utils.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;

namespace fractals::common
{

TEST(UTILS, mapVector)
{
    std::vector<int> v{1, 2, 3, 4};
    std::function<int(int)> addOne = [](int x) -> int
    {
        return x + 1;
    };

    std::vector<int> res = mapVector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2, 3, 4, 5));
}

TEST(UTILS, concat)
{
    std::vector<std::string> v{"1234", "ery", "", "mnbv"};
    auto res = concat(v);

    ASSERT_EQ(res, "1234erymnbv");
}

TEST(UTILS, intercalate)
{
    std::vector<std::string> v{"1234", "ery", "", "mnbv"};
    auto res = intercalate(",", v);

    ASSERT_EQ(res, "1234,ery,,mnbv");
}

TEST(UTILS, makeSizedLine)
{
    std::string s("1234567");
    auto res = makeSizedLine(s, 4);

    ASSERT_EQ(res, "1234\n");
}

TEST(UTILS, elem)
{
    std::string val("a");

    auto res1 = elem(val, "b", "c", "a", "d");

    ASSERT_TRUE(res1);

    auto res2 = elem(val, "b", "c", "d");

    ASSERT_FALSE(res2);
}

TEST(UTILS, random_alphaNumerical)
{
    int length = 10;
    auto res = randomAlphaNumerical(length);

    ASSERT_TRUE(res.length() == length);

    for (auto &c : res)
    {
        ASSERT_TRUE(std::isalpha(c) || std::isdigit(c));
    }
}

TEST(UTILS, bytesToHex)
{
    std::vector<char> bytes{'\xde', '\xad', '\xbe', '\xef'};

    auto res = bytesToHex(bytes);

    ASSERT_EQ(res, "deadbeef");
}

TEST(UTILS, hexToBytes)
{
    std::string hex{"deadbeef"};

    auto res = hexToBytes(hex);

    EXPECT_THAT(res, testing::ElementsAre('\xde', '\xad', '\xbe', '\xef'));
}

TEST(UTILS, intToBytes)
{
    auto res1 = intToBytes(230530);
    auto res2 = intToBytes(0);
    auto res3 = intToBytes(-501);

    EXPECT_THAT(res1, testing::ElementsAre('\x00', '\x03', '\x84', '\x82'));
    EXPECT_THAT(res2, testing::ElementsAre('\x00', '\x00', '\x00', '\x00'));
    EXPECT_THAT(res3, testing::ElementsAre('\xFF', '\xFF', '\xFE', '\v'));
}

TEST(UTILS, bytesToInt)
{
    std::deque<char> d1{'\x00', '\x03', '\x84', '\x82'};
    std::deque<char> d2{'\x00', '\x00', '\x00', '\x00'};
    std::deque<char> d3{'\xFF', '\xFF', '\xFE', '\v'};

    auto res1 = bytesToInt(d1);
    auto res2 = bytesToInt(d2);
    auto res3 = bytesToInt(d3);

    ASSERT_EQ(res1, 230530);
    ASSERT_EQ(res2, 0);
    ASSERT_EQ(res3, -501);
}

TEST(UTILS, bitfieldToBytes)
{
    std::vector<bool> bits1;
    std::vector<bool> bits2{0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<bool> bits3{1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1};
    std::vector<bool> bits4{1, 1, 1, 1, 1, 1, 1, 1};

    auto res1 = bitfieldToBytes(bits1);
    auto res2 = bitfieldToBytes(bits2);
    auto res3 = bitfieldToBytes(bits3);
    auto res4 = bitfieldToBytes(bits4);

    ASSERT_TRUE(res1.size() == 0);
    EXPECT_THAT(res2, testing::ElementsAre('\x00'));
    EXPECT_THAT(res3, testing::ElementsAre('\x98', '\xC4'));
    EXPECT_THAT(res4, testing::ElementsAre('\xFF'));
}

TEST(UTILS, bytesToBitfield)
{
    std::vector<char> bytes1;
    std::vector<char> bytes2{'\x00'};
    std::vector<char> bytes3{'\x98', '\xC4'};
    std::vector<char> bytes4{'\xFF'};

    std::string_view v1(bytes1.begin(), bytes1.end());
    std::string_view v2(bytes2.begin(), bytes2.end());
    std::string_view v3(bytes3.begin(), bytes3.end());
    std::string_view v4(bytes4.begin(), bytes4.end());

    auto res1 = bytesToBitfield(0, v1);
    auto res2 = bytesToBitfield(1, v2);
    auto res3 = bytesToBitfield(2, v3);
    auto res4 = bytesToBitfield(1, v4);

    ASSERT_TRUE(res1.size() == 0);
    EXPECT_THAT(res2, testing::ElementsAre(0, 0, 0, 0, 0, 0, 0, 0));
    EXPECT_THAT(res3, testing::ElementsAre(1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0));
    EXPECT_THAT(res4, testing::ElementsAre(1, 1, 1, 1, 1, 1, 1, 1));
}

TEST(UTILS, ppBytes)
{
    int64_t i1 = 0;
    int64_t i2 = 2000;
    int64_t i3 = 4538945;
    int64_t i4 = 903478923489723423;

    auto res1 = ppBytes(i1);
    auto res2 = ppBytes(i2);
    auto res3 = ppBytes(i3);
    auto res4 = ppBytes(i4);

    ASSERT_EQ(res1, "0.0 B");
    ASSERT_EQ(res2, "2.0 KB");
    ASSERT_EQ(res3, "4.53 MB");
    ASSERT_EQ(res4, "903.47 PB");
}

TEST(UTILS, ppBytesPerSecond)
{
    int64_t i = 903478923489723423;

    auto res = ppBytesPerSecond(i);

    ASSERT_EQ(res, "903.47 PB/s");
}

TEST(UTILS, ppTime)
{
    int64_t i1 = 0;
    int64_t i2 = 12;
    int64_t i3 = 60; // minute
    int64_t i4 = i3 + (i2 * 20) + 10;
    int64_t i5 = i3 * 60; // hour
    int64_t i6 = i5 + (i4 * 2);
    int64_t i7 = i5 * 24; // day
    int64_t i8 = i7 + (i6 * 3);
    int64_t i9 = i7 * 365; // year
    int64_t i10 = i9 + (i8 * 40);
    int64_t i11 = i10 * 1000;

    auto res1 = ppTime(i1);
    auto res2 = ppTime(i2);
    auto res3 = ppTime(i3);
    auto res4 = ppTime(i4);
    auto res5 = ppTime(i5);
    auto res6 = ppTime(i6);
    auto res7 = ppTime(i7);
    auto res8 = ppTime(i8);
    auto res9 = ppTime(i9);
    auto res10 = ppTime(i10);
    auto res11 = ppTime(i11);

    ASSERT_EQ(res1, "0s");
    ASSERT_EQ(res2, "12s");
    ASSERT_EQ(res3, "1m0s");
    ASSERT_EQ(res4, "5m10s");
    ASSERT_EQ(res5, "1h0m0s");
    ASSERT_EQ(res6, "1h10m20s");
    ASSERT_EQ(res7, "1d0h0m0s");
    ASSERT_EQ(res8, "1d3h31m0s");
    ASSERT_EQ(res9, "1y0m5d0h0m0s");
    ASSERT_EQ(res10, "1y1m20d20h40m0s");
    ASSERT_EQ(res11, "inf");
}

} // namespace Fractals::common