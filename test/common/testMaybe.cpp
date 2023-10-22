#include <fractals/common/maybe.h>
#include <neither/either.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

using ::testing::_;
using namespace std::string_literals;

namespace fractals::common
{

TEST(MAYBE, mmapVector_maybe_success)
{
    std::vector<int> v{1, 2, 3, 4};
    auto addOne = [](int x) -> neither::Maybe<int>
    {
        return x + 1;
    };
    auto res = mmapVector<int, int>(v, addOne);

    ASSERT_TRUE(res);
    EXPECT_THAT(res.value, ::testing::ElementsAre(2, 3, 4, 5));
}

TEST(MAYBE, mmapVector_maybe_failure)
{
    std::vector<int> v{1, 2, 3, 4};
    auto evenOnly = [](int x) -> neither::Maybe<int>
    {
        if (x % 2 == 0)
        {
            return x;
        }
        else
        {
            return {};
        }
    };
    auto res = mmapVector<int, int>(v, evenOnly);

    ASSERT_FALSE(res);
}

TEST(MAYBE, mmapVector_either_success)
{
    std::vector<int> v{1, 2, 3, 4};
    auto addOne = [](int x) -> neither::Either<std::string, int>
    {
        return neither::right<int>(x + 1);
    };
    auto res = mmapVector<int, std::string, int>(v, addOne);

    ASSERT_TRUE(!res.isLeft);
    EXPECT_THAT(res.rightValue, ::testing::ElementsAre(2, 3, 4, 5));
}

TEST(MAYBE, mmapVector_either_failure)
{
    std::vector<int> v{1, 2, 3, 4};
    auto evenOnly = [](int x) -> neither::Either<std::string, int>
    {
        if (x % 2 == 0)
        {
            return neither::right<int>(x);
        }
        else
        {
            return neither::left<std::string>("fail");
        }
    };
    auto res = mmapVector<int, std::string, int>(v, evenOnly);

    ASSERT_TRUE(res.isLeft);
    ASSERT_EQ(res.leftValue, std::string("fail"));
}

TEST(MAYBE, choice_maybe)
{
    {
        neither::Maybe<int> m1;
        neither::Maybe<int> m2(2);
        auto res = choice(m1, m2);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value, 2);
    }
    {
        neither::Maybe<int> m1(1);
        neither::Maybe<int> m2;
        auto res = choice(m1, m2);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value, 1);
    }
    {
        neither::Maybe<int> m1;
        neither::Maybe<int> m2;
        auto res = choice(m1, m2);
        ASSERT_FALSE(res);
    }
}

TEST(MAYBE, choice_either)
{
    {
        neither::Either<std::string, int> m1 = neither::left<std::string>("f1"s);
        neither::Either<std::string, int> m2 = neither::right(2);
        auto res = choice(m1, m2);
        ASSERT_TRUE(!res.isLeft);
        ASSERT_EQ(res.rightValue, 2);
    }
    {
        neither::Either<std::string, int> m1 = neither::right(1);
        neither::Either<std::string, int> m2 = neither::left("f2"s);
        auto res = choice(m1, m2);
        ASSERT_TRUE(!res.isLeft);
        ASSERT_EQ(res.rightValue, 1);
    }
    {
        neither::Either<std::string, int> m1 = neither::left("f1"s);
        neither::Either<std::string, int> m2 = neither::left("f2"s);
        auto res = choice(m1, m2);
        ASSERT_TRUE(res.isLeft);
        ASSERT_EQ(res.leftValue, "f2");
    }
}

TEST(MAYBE, maybeToEither)
{
    {
        neither::Maybe<int> m(1);
        std::string s("fail");
        auto res = maybeToEither(m, s);

        ASSERT_TRUE(!res.isLeft);
        ASSERT_EQ(res.rightValue, 1);
    }
    {
        neither::Maybe<int> m;
        std::string s("fail");
        auto res = maybeToEither(m, s);

        ASSERT_TRUE(res.isLeft);
        ASSERT_EQ(res.leftValue, "fail");
    }
}

TEST(MAYBE, fromMaybe)
{
    {
        neither::Maybe<int> m(1);
        auto res = fromMaybe(m, 2);
        ASSERT_EQ(res, 1);
    }
    {
        neither::Maybe<int> m;
        auto res = fromMaybe(m, 2);
        ASSERT_EQ(res, 2);
    }
}

TEST(MAYBE, maybeToVal)
{
    auto toString = [](int x)
    {
        return std::to_string(x);
    };
    {
        neither::Maybe<int> m(1);
        auto res = maybeToVal<int, std::string>(m, toString, "2");
        ASSERT_EQ(res, "1");
    }
    {
        neither::Maybe<int> m;
        auto res = maybeToVal<int, std::string>(m, toString, "2");
        ASSERT_EQ(res, "2");
    }
}

TEST(MAYBE, eitherToVal)
{
    auto identity = [](std::string s) -> std::string
    {
        return s;
    };
    auto toString = [](int x) -> std::string
    {
        return std::to_string(x);
    };
    {
        std::variant<std::string, int> e{1};
        auto res = eitherToVal<std::string, int, std::string>(e, identity, toString);
        ASSERT_EQ(res, "1");
    }
    {
        std::variant<std::string, int> e{"fail"s};
        auto res = eitherToVal<std::string, int, std::string>(e, identity, toString);
        ASSERT_EQ(res, "fail");
    }
}

TEST(MAYBE, eitherOf)
{
    {
        neither::Either<std::string, int> e(neither::right(1));
        neither::Either<std::string, bool> e2(neither::left("fail2"s));
        auto res = eitherOf(e, e2);
        ASSERT_TRUE(!res.isLeft);
        ASSERT_TRUE(res.rightValue.isLeft);
        ASSERT_EQ(res.rightValue.leftValue, 1);
    }
    {
        neither::Either<std::string, int> e(neither::left("fail"s));
        neither::Either<std::string, bool> e2(neither::right(false));
        auto res = eitherOf(e, e2);
        ASSERT_TRUE(!res.isLeft);
        ASSERT_TRUE(!res.rightValue.isLeft);
        ASSERT_EQ(res.rightValue.rightValue, false);
    }
    {
        neither::Either<std::string, int> e(neither::left("fail"s));
        neither::Either<std::string, bool> e2(neither::left("fail2"s));
        auto res = eitherOf(e, e2);
        ASSERT_TRUE(res.isLeft);
        ASSERT_EQ(res.leftValue, "fail2");
    }
}

TEST(MAYBE, mapEither)
{
    std::vector<int> v0;
    std::vector<int> v1{1, 2, 3, 4};
    auto onEven = [](int x) -> neither::Either<std::string, std::string>
    {
        if (x % 2 == 0)
        {
            return neither::right(std::to_string(x));
        }
        else
        {
            return neither::left("fail"s);
        }
    };

    {
        auto res = mapEither<std::string, int, std::string>(v0, onEven);
        EXPECT_THAT(res, ::testing::ElementsAre());
    }
    {
        auto res = mapEither<std::string, int, std::string>(v1, onEven);
        EXPECT_THAT(res, ::testing::ElementsAre("2", "4"));
    }
}

} // namespace fractals::common