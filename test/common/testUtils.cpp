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
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}

TEST(UTILS, bytes_to_bitfield)
{
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}

TEST(UTILS, pp_bytes)
{
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}

TEST(UTILS, pp_bytes_per_second)
{
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}

TEST(UTILS, pp_time)
{
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}

TEST(UTILS, make_wide)
{
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}

TEST(UTILS, unwide)
{
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}

TEST(UTILS, print_err)
{
    std::vector<int> v{1,2,3,4};
    std::function<int(int)> addOne = [](int x) -> int {
        return x + 1;
    };

    std::vector<int> res = fractals::common::map_vector(v, addOne);

    EXPECT_THAT(res, testing::ElementsAre(2,3,4,5));
}