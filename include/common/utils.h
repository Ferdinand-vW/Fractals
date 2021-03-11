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

std::string concat_paths(std::vector<std::string> v);

std::string random_alphaNumerical(int length);

std::string bytes_to_hex(const std::vector<char> &bytes);
std::string bytes_to_hex(const std::deque<char> &bytes);

std::vector<char> int_to_bytes(int n);

int bytes_to_int(std::deque<char> &d);

std::vector<char> bitfield_to_bytes(const std::vector<bool> &bitfields);
std::vector<bool> bytes_to_bitfield(int len,std::deque<char> &bitfields);