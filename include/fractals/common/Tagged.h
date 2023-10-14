#pragma once

#include "fractals/common/utils.h"
#include <array>
#include <cstring>
#include <istream>
#include <string>

#include "spdlog/fmt/ostr.h"

namespace fractals::common
{

struct InfoHash
{
    constexpr InfoHash(){};
    constexpr InfoHash(std::string_view str)
    {
        std::copy(str.begin(), str.end(), underlying.data());
    }
    constexpr InfoHash(const std::array<char, 20> &underlying) : underlying(underlying)
    {
    }

    bool operator==(const InfoHash &h) const
    {
        return underlying == h.underlying;
    }

    friend std::ostream &operator<<(std::ostream &os, const InfoHash &hash)
    {
        common::string_view view(hash.underlying.begin(), hash.underlying.begin() + 5);
        return os << bytes_to_hex(view);
    }

    std::string toString() const
    {
        return std::string(underlying.begin(), underlying.end());
    }

    std::array<char, 20> underlying{};
};

struct PieceHash
{
    PieceHash(){};
    PieceHash(const std::string &str)
    {
        std::copy(str.begin(), str.end(), underlying.data());
    }
    PieceHash(const std::vector<char> &vec)
    {
        std::copy(vec.begin(), vec.end(), underlying.data());
    }
    PieceHash(const std::array<char, 20> &underlying) : underlying(underlying)
    {
    }

    std::array<char, 20> underlying;

    bool operator==(const InfoHash &h) const
    {
        return underlying == h.underlying;
    }

    friend std::ostream &operator<<(std::ostream &os, const PieceHash &hash)
    {
        return os << bytes_to_hex<20>(hash.underlying);
    }
};

} // namespace fractals::common

namespace std
{
template <> struct std::hash<fractals::common::InfoHash>
{
    std::size_t operator()(const fractals::common::InfoHash &k) const
    {
        return std::hash<std::string>{}(std::string(k.underlying.begin(), k.underlying.end()));
    }
};
} // namespace std