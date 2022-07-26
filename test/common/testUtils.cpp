#include "gmock/gmock.h"
#include <cctype>
#include <fractals/common/utils.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;

TEST(UTILS, map_vector)
{
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}

TEST(UTILS, concat)
{
    std::vector<std::string> v{"1234","ery","","mnbv"};
    auto res = fractals::common::concat(v);

    ASSERT_EQ(res, "1234erymnbv");
}

TEST(UTILS, intercalate)
{
    std::vector<std::string> v{"1234","ery","","mnbv"};
    auto res = fractals::common::intercalate(",", v);

    ASSERT_EQ(res, "1234,ery,,mnbv");
}

TEST(UTILS, make_sized_line)
{
   std::string s("1234567");
   auto res = fractals::common::make_sized_line(s, 4);

   ASSERT_EQ(res, "1234\n");
}

TEST(UTILS, elem)
{
    std::string val("a");

    auto res1 = fractals::common::elem(val, "b", "c", "a", "d");

    ASSERT_TRUE(res1);

    auto res2 = fractals::common::elem(val, "b", "c", "d");
    
    ASSERT_FALSE(res2);
}

TEST(UTILS, random_alphaNumerical)
{
    int length = 10;
    auto res = fractals::common::random_alphaNumerical(length);

    ASSERT_TRUE(res.length() == length);
    
    for (auto &c : res)
    {
        ASSERT_TRUE(std::isalpha(c) || std::isdigit(c));
    }
}

TEST(UTILS, bytes_to_hex)
{
    std::vector<char> bytes{'\xde','\xad','\xbe','\xef'};

    auto res = fractals::common::bytes_to_hex(bytes);

    ASSERT_EQ(res, "deadbeef");
}

TEST(UTILS, int_to_bytes)
{
    auto res1 = fractals::common::int_to_bytes(230530);
    auto res2 = fractals::common::int_to_bytes(0);
    auto res3 = fractals::common::int_to_bytes(-501);

    EXPECT_THAT(res1, testing::ElementsAre('\x00','\x03','\x84','\x82'));
    EXPECT_THAT(res2, testing::ElementsAre('\x00','\x00','\x00','\x00'));
    EXPECT_THAT(res3, testing::ElementsAre('\xFF','\xFF','\xFE','\v'));
}

TEST(UTILS, bytes_to_int)
{
    std::deque<char> d1{'\x00','\x03','\x84','\x82'};
    std::deque<char> d2{'\x00','\x00','\x00','\x00'};
    std::deque<char> d3{'\xFF','\xFF','\xFE','\v'};
    
    auto res1 = fractals::common::bytes_to_int(d1);
    auto res2 = fractals::common::bytes_to_int(d2);
    auto res3 = fractals::common::bytes_to_int(d3);

    ASSERT_EQ(res1, 230530);
    ASSERT_EQ(res2, 0);
    ASSERT_EQ(res3, -501);
}

TEST(UTILS, bitfield_to_bytes)
{
    std::vector<bool> bits1;
    std::vector<bool> bits2{0,0,0,0,0,0,0,0};
    std::vector<bool> bits3{1,0,0,1,1,0,0,0,1,1,0,0,0,1};
    std::vector<bool> bits4{1,1,1,1,1,1,1,1};

    auto res1 = fractals::common::bitfield_to_bytes(bits1);
    auto res2 = fractals::common::bitfield_to_bytes(bits2);
    auto res3 = fractals::common::bitfield_to_bytes(bits3);
    auto res4 = fractals::common::bitfield_to_bytes(bits4);

    ASSERT_TRUE(res1.size() == 0);
    EXPECT_THAT(res2, testing::ElementsAre('\x00'));
    EXPECT_THAT(res3, testing::ElementsAre('\x98','\xC4'));
    EXPECT_THAT(res4, testing::ElementsAre('\xFF'));
}

TEST(UTILS, bytes_to_bitfield)
{
    std::vector<char> bytes1;
    std::vector<char> bytes2{'\x00'};
    std::vector<char> bytes3{'\x98','\xC4'};
    std::vector<char> bytes4{'\xFF'};

    auto res1 = fractals::common::bytes_to_bitfield(0, bytes1);
    auto res2 = fractals::common::bytes_to_bitfield(1, bytes2);
    auto res3 = fractals::common::bytes_to_bitfield(2, bytes3);
    auto res4 = fractals::common::bytes_to_bitfield(1, bytes4);

    ASSERT_TRUE(res1.size() == 0);
    EXPECT_THAT(res2, testing::ElementsAre(0,0,0,0,0,0,0,0));
    EXPECT_THAT(res3, testing::ElementsAre(1,0,0,1,1,0,0,0,1,1,0,0,0,1,0,0));
    EXPECT_THAT(res4, testing::ElementsAre(1,1,1,1,1,1,1,1));
}

TEST(UTILS, pp_bytes)
{
    int64_t i1 = 0;
    int64_t i2 = 2000;
    int64_t i3 = 4538945;
    int64_t i4 = 903478923489723423;

    auto res1 = fractals::common::pp_bytes(i1);
    auto res2 = fractals::common::pp_bytes(i2);
    auto res3 = fractals::common::pp_bytes(i3);
    auto res4 = fractals::common::pp_bytes(i4);

    ASSERT_EQ(res1, L"0.0 B");
    ASSERT_EQ(res2, L"2.0 KB");
    ASSERT_EQ(res3, L"4.53 MB");
    ASSERT_EQ(res4, L"903.47 PB");
}

TEST(UTILS, pp_bytes_per_second)
{
    int64_t i = 903478923489723423;

    auto res = fractals::common::pp_bytes_per_second(i);

    ASSERT_EQ(res, L"903.47 PB/s");
}

TEST(UTILS, pp_time)
{
    int64_t i1 = 0;
    int64_t i2 = 12;
    int64_t i3 = 60; // minute
    int64_t i4 = i3 + (i2 * 20) + 10;
    int64_t i5 = i3*60 ; // hour
    int64_t i6 = i5 + (i4 * 2);
    int64_t i7 = i5 * 24; // day
    int64_t i8 = i7 + (i6 * 3);
    int64_t i9 = i7 * 365; // year
    int64_t i10 = i9 + (i8 * 40);
    int64_t i11 = i10 * 1000;

    auto res1 = fractals::common::pp_time(i1);
    auto res2 = fractals::common::pp_time(i2);
    auto res3 = fractals::common::pp_time(i3);
    auto res4 = fractals::common::pp_time(i4);
    auto res5 = fractals::common::pp_time(i5);
    auto res6 = fractals::common::pp_time(i6);
    auto res7 = fractals::common::pp_time(i7);
    auto res8 = fractals::common::pp_time(i8);
    auto res9 = fractals::common::pp_time(i9);
    auto res10 = fractals::common::pp_time(i10);
    auto res11 = fractals::common::pp_time(i11);

    ASSERT_EQ(res1, L"0s");
    ASSERT_EQ(res2, L"12s");
    ASSERT_EQ(res3, L"1m0s");
    ASSERT_EQ(res4, L"5m10s");
    ASSERT_EQ(res5, L"1h0m0s");
    ASSERT_EQ(res6, L"1h10m20s");
    ASSERT_EQ(res7, L"1d0h0m0s");
    ASSERT_EQ(res8, L"1d3h31m0s");
    ASSERT_EQ(res9, L"1y0m5d0h0m0s");
    ASSERT_EQ(res10, L"1y1m20d20h40m0s");
    ASSERT_EQ(res11, L"inf");
}
