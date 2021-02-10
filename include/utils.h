#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>
#include <ostream>
#include <vector>

using namespace std;

template <class A,class B>
static std::vector<B> map_vector(std::vector<A> v,std::function<B(A)> f) {
    std::vector<B> vec_out;
    std::transform(v.begin(),v.end(),std::back_inserter(vec_out),f);
    return vec_out;
}

static std::string str_concat_vector(std::vector<std::string> v) {
    std::string out("");
    for(auto s : v) {
        out.append(s);
    }
    return out;
}

static std::string intercalate(std::string del,std::vector<std::string> v) {
    std::string out("");
    bool first = true;
    for(auto s : v) {
        if (first) { out.append(s); first = false; }
        else       { out.append(del + s); }
    }
    return out;
}

static string make_sized_line(string s,int len) {
    return s.substr(0,len) + "\n";
}
