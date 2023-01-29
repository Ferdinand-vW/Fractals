#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <set>
#include <vector>

namespace fractals::common {

using string_view = std::basic_string_view<char>;

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/**
Apply function to each member of vector
*/
template <class A, class B>
std::vector<B> map_vector(const std::vector<A> &v, std::function<B(A)> f) {
  std::vector<B> vec_out;
  std::transform(v.begin(), v.end(), std::back_inserter(vec_out), f);
  return vec_out;
}

template <typename>
struct GetArraySize;
template <typename A, size_t N>
struct GetArraySize<std::array<A, N>>
{
  constexpr static size_t SIZE = N;
};

template <typename Container>
size_t capacity(const Container& c)
{
  if constexpr (std::is_same_v<Container, std::basic_string<typename Container::value_type>>)
  {
    return c.capacity();
  }
  else if constexpr (std::is_same_v<Container, std::basic_string_view<typename Container::value_type>>)
  {
    return c.size();
  }
  else if constexpr (std::is_same_v<Container, std::vector<typename Container::value_type>>)
  {
    return c.capacity();
  }
  else if constexpr (std::is_same_v<Container, std::array<typename Container::value_type, GetArraySize<Container>::SIZE>>)
  {
    return c.max_size();
  }
}

template <class Container1, class Container2>
void append(Container1& v1, const Container2& v2)
{
  if (std::max(capacity<Container1>(v1), capacity<Container2>(v2)) < v1.size() + v2.size())
  {
    v1.reserve(v1.size() + v2.size());
  }
  v1.insert(v1.end(), v2.begin(), v2.end());
}

/**
Turns a vector of strings into a single string without delimiters
*/
std::string concat(const std::vector<std::string> &v);
std::string concat(const std::vector<char>& cs);

/**
Turns a vector of strings into a single string delimited by @del
*/
std::string intercalate(std::string del, const std::vector<std::string> &v);
std::string intercalate(std::string del, const std::vector<char> &v);
std::string intercalate(std::string del, const std::deque<char> &v);

/**
Remove characters from string after a certain size
*/
std::string make_sized_line(std::string s, int len);

/**
Check whether @f is equal to one of @t
*/
template <typename First, typename... T> bool elem(First &&f, T &&... t) {
  return ((f == t) || ...);
}

/**
Returns a randomly generated alphanumerical string of length @length
*/
std::string random_alphaNumerical(int length);

/**
Convert bytes to hex format
*/
std::string bytes_to_hex(const std::vector<char> &bytes);
std::string bytes_to_hex(const std::deque<char> &bytes);

/**
Convert hex format to bytes
*/
std::vector<char> hex_to_bytes(const std::string& s);

//int is 4 bytes
//if int n consist of bytes W X Y Z then converting to char leaves us with Z
// v.push_back(n >> 24); //Move W to 0 0 0 W
// v.push_back(n >> 16); //Move X to 0 0 W X
// v.push_back(n >> 8); //Move Y to 0 W X Y
// v.push_back(n); //Z is already in right location
template <class T>
std::vector<char> int_to_bytes(T t)  {
    auto size = sizeof(T);
    std::vector<char> v;
    v.reserve(size);

    while(size > 0)
    {
        --size;
        v.emplace_back(t >> 8 * size);
    }

    return v;
}

/**
Parse bytes as int
*/
template <typename T>
T bytes_to_int(common::string_view &d)
{
    if (d.size() < 1) { return {}; }

    T n = 0;
    int size = d.size() > sizeof(T) ? sizeof(T) : d.size(); //We may have fewer than 4 bytes
    for(int i = 0; i < size; i++) {
        //Example:
        //Assume size = 3 (X Y Z) and start with 0 0 0 0, then
        //byte X must be moved 16bits left: 3*8 - (0+1)*8 = 16, 0 X 0 0
        //byte Y must be moved 8bits left: 3*8 - (1+1)*8 = 8, 0 X Y 0
        //byte Z must be moved 0bits left: 3*8 - (2+1)*8 = 0, 0 X Y Z
        // note that the | operator does or operations between bytes:
        // 0 X 0 0
        // 0 0 Y 0
        // ------- or (|)
        // 0 X Y 0
        n |= (unsigned char)d.front() << (size*8 - (i+1) * 8);
        d.remove_prefix(1);
    }

    return n;
}
int bytes_to_int(std::deque<char> &d);

/**
BitTorrent bitfield to byte representation
*/
std::vector<char> bitfield_to_bytes(const std::vector<bool> &bitfields);
/**
Byte representation of BitTorrent bitfield to actual
*/
std::vector<bool> bytes_to_bitfield(int len, std::deque<char> &bitfields);
std::vector<bool> bytes_to_bitfield(int len, common::string_view bitfields);

std::wstring pp_bytes(int64_t bytes);
std::wstring pp_bytes_per_second(int64_t bytes);
std::wstring pp_time(int64_t seconds);
std::wstring make_wide(const std::string &s);
std::string unwide(const std::wstring &ws);

template <typename A>
std::unordered_set<A> setDifference(const std::unordered_set<A>& c1, const std::unordered_set<A>& c2)
{
    std::unordered_set<A> s;

    for(const auto& a : c1)
    {
        if (!c2.count(a))
        {
            s.emplace(a);
        }
    }

    return s;
}

template <typename Container>
std::unordered_set<typename Container::key_type> keysOfMap(const Container& c)
{
    std::unordered_set<typename Container::key_type> s;

    for(const auto& kvp : c)
    {
        s.emplace(kvp.first);
    }

    return s;
}

void print_err(std::string &&s);

} // namespace fractals::common