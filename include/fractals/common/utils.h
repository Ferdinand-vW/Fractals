#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
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

/**
Convert int to bytes
*/
std::vector<char> int_to_bytes(uint32_t n);

/**
Parse bytes as int
*/
int bytes_to_int(std::deque<char> &d);

/**
BitTorrent bitfield to byte representation
*/
std::vector<char> bitfield_to_bytes(const std::vector<bool> &bitfields);
/**
Byte representation of BitTorrent bitfield to actual
*/
std::vector<bool> bytes_to_bitfield(int len, std::deque<char> &bitfields);
std::vector<bool> bytes_to_bitfield(int len, std::vector<char> &bitfields);

std::wstring pp_bytes(int64_t bytes);
std::wstring pp_bytes_per_second(int64_t bytes);
std::wstring pp_time(int64_t seconds);
std::wstring make_wide(const std::string &s);
std::string unwide(const std::wstring &ws);

void print_err(std::string &&s);

} // namespace fractals::common