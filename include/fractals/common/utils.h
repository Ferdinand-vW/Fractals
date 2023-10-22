#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <iomanip>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace fractals::common
{

// helper type for the visitor #4
template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/**
Apply function to each member of vector
*/
template <class A, class B>
std::vector<B> mapVector(const std::vector<A> &v, std::function<B(A)> f)
{
    std::vector<B> result;
    std::transform(v.begin(), v.end(), std::back_inserter(result), f);
    return result;
}

template <typename> struct GetArraySize;
template <typename A, size_t N> struct GetArraySize<std::array<A, N>> {
    constexpr static size_t SIZE = N;
};

template <typename Container> size_t capacity(const Container &c)
{
    if constexpr (std::is_same_v<Container, std::basic_string<typename Container::value_type>>) {
        return c.capacity();
    } else if constexpr (std::is_same_v<Container,
                                        std::basic_string_view<typename Container::value_type>>) {
        return c.size();
    } else if constexpr (std::is_same_v<Container, std::vector<typename Container::value_type>>) {
        return c.capacity();
    } else if constexpr (std::is_same_v<Container, std::array<typename Container::value_type,
                                                              GetArraySize<Container>::SIZE>>) {
        return c.max_size();
    }
}

template <class Container1, class Container2> void append(Container1 &v1, const Container2 &v2)
{
    if (std::max(capacity<Container1>(v1), capacity<Container2>(v2)) < v1.size() + v2.size()) {
        v1.reserve(v1.size() + v2.size());
    }
    v1.insert(v1.end(), v2.begin(), v2.end());
}

/**
Turns a vector of strings into a single string without delimiters
*/
std::string concat(const std::vector<std::string> &v);
std::string concat(const std::vector<char> &cs);
template <int N>
std::string concat(const std::array<char, N> arr)
{
    return std::string(arr.begin(), arr.end());
}

/**
Turns a vector of strings into a single string delimited by @del
*/
std::string intercalate(std::string del, const std::vector<std::string> &v);
std::string intercalate(std::string del, const std::vector<char> &v);
std::string intercalate(std::string del, const std::deque<char> &v);

/**
Remove characters from string after a certain size
*/
std::string makeSizedLine(std::string s, int len);

/**
Check whether @f is equal to one of @t
*/
template <typename First, typename... T> bool elem(First &&f, T &&...t)
{
    return ((f == t) || ...);
}

/**
Returns a randomly generated alphanumerical string of length @length
*/
std::string randomAlphaNumerical(int length);

/**
Convert bytes to hex format
*/
std::string bytesToHex(std::string_view view);
std::string bytesToHex(const std::vector<char> &bytes);
template <int N> std::string bytesToHex(const std::array<char, N> &bytes)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto &b : bytes) {
        ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(b));
    };

    return ss.str();
}
std::string bytesToHex(const std::deque<char> &bytes);

/**
Convert hex format to bytes
*/
std::vector<char> hexToBytes(const std::string &s);

// int is 4 bytes
// if int n consist of bytes W X Y Z then converting to char leaves us with Z
//  v.push_back(n >> 24); //Move W to 0 0 0 W
//  v.push_back(n >> 16); //Move X to 0 0 W X
//  v.push_back(n >> 8); //Move Y to 0 W X Y
//  v.push_back(n); //Z is already in right location
template <class T> std::vector<char> intToBytes(T t)
{
    auto size = sizeof(T);
    std::vector<char> v;
    v.reserve(size);

    while (size > 0) {
        --size;
        v.emplace_back(t >> 8 * size);
    }

    return v;
}

/**
Parse bytes as int
*/
template <typename T> T bytesToInt(std::string_view &d)
{
    if (d.size() < 1) {
        return {};
    }

    T n = 0;
    int size = d.size() > sizeof(T) ? sizeof(T) : d.size(); // We may have fewer than 4 bytes
    for (int i = 0; i < size; i++) {
        // Example:
        // Assume size = 3 (X Y Z) and start with 0 0 0 0, then
        // byte X must be moved 16bits left: 3*8 - (0+1)*8 = 16, 0 X 0 0
        // byte Y must be moved 8bits left: 3*8 - (1+1)*8 = 8, 0 X Y 0
        // byte Z must be moved 0bits left: 3*8 - (2+1)*8 = 0, 0 X Y Z
        //  note that the | operator does or operations between bytes:
        //  0 X 0 0
        //  0 0 Y 0
        //  ------- or (|)
        //  0 X Y 0
        n |= (unsigned char)d.front() << (size * 8 - (i + 1) * 8);
        d.remove_prefix(1);
    }

    return n;
}
int bytesToInt(std::deque<char> &d);

/**
BitTorrent bitfield to byte representation
*/
std::vector<char> bitfieldToBytes(const std::vector<bool> &bitfields);
/**
Byte representation of BitTorrent bitfield to actual
*/
std::vector<bool> bytesToBitfield(int len, std::deque<char> &bitfields);
std::vector<bool> bytesToBitfield(int len, std::string_view bitfields);

std::string ppBytes(int64_t bytes);
std::string ppBytesPerSecond(int64_t bytes);
std::string ppTime(int64_t seconds);

template <typename T>
std::string toString(const T& t)
{
    std::stringstream ss;
    ss << t;

    return ss.str();
}

template <typename A>
std::unordered_set<A> setDifference(const std::unordered_set<A> &c1,
                                    const std::unordered_set<A> &c2)
{
    std::unordered_set<A> s;

    for (const auto &a : c1) {
        if (!c2.count(a)) {
            s.emplace(a);
        }
    }

    return s;
}

template <typename Container>
std::unordered_set<typename Container::key_type> keysOfMap(const Container &c)
{
    std::unordered_set<typename Container::key_type> s;

    for (const auto &kvp : c) {
        s.emplace(kvp.first);
    }

    return s;
}

void printErr(std::string &&s);

} // namespace fractals::common