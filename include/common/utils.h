#pragma once

#include "neither/maybe.hpp"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <vector>
#include <functional>

template <class A,class B>
std::vector<B> map_vector(std::vector<A> v,std::function<B(A)> f) {
    std::vector<B> vec_out;
    std::transform(v.begin(),v.end(),std::back_inserter(vec_out),f);
    return vec_out;
}

std::string str_concat_vector(std::vector<std::string> v);

std::string intercalate(std::string del,std::vector<std::string> v);

std::string make_sized_line(std::string s,int len);

