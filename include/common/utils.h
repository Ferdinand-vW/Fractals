#pragma once

#include "neither/maybe.hpp"
#include <algorithm>
#include <deque>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <vector>
#include <functional>

template <class A,class B>
std::vector<B> map_vector(const std::vector<A> &v,std::function<B(A)> f) {
    std::vector<B> vec_out;
    std::transform(v.begin(),v.end(),std::back_inserter(vec_out),f);
    return vec_out;
}

std::string str_concat_vector(const std::vector<std::string> &v);

std::string intercalate(std::string del,const std::vector<std::string> &v);

std::string make_sized_line(std::string s,int len);
std::wstring take_n(std::wstring s, int n);
std::string take_n(std::string s, int n);

std::string concat_paths(std::vector<std::string> v);

template<typename First,typename ... T>
bool elem(First &&f,T && ... t) {
    return ((f == t) || ...);
}

std::string random_alphaNumerical(int length);

std::string bytes_to_hex(const std::vector<char> &bytes);
std::string bytes_to_hex(const std::deque<char> &bytes);

std::vector<char> int_to_bytes(int n);

int bytes_to_int(std::deque<char> &d);

std::vector<char> bitfield_to_bytes(const std::vector<bool> &bitfields);
std::vector<bool> bytes_to_bitfield(int len,std::deque<char> &bitfields);
std::vector<bool> bytes_to_bitfield(int len,std::vector<char> &bitfields);

std::wstring pp_bytes(long long bytes);
std::wstring pp_bytes_per_second(long long bytes);
std::wstring make_wide(const std::string &s);
std::string unwide(const std::wstring &ws);