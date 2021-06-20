
#include "common/utils.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <list>
#include <string>
#include <sstream>
#include <filesystem>
#include <neither/neither.hpp>

std::string str_concat_vector(const std::vector<std::string> &v) {
    std::string out("");
    for(auto s : v) {
        out.append(s);
    }
    return out;
}

std::string intercalate(std::string del,const std::vector<std::string> &v) {
    std::string out("");
    bool first = true;
    for(auto s : v) {
        if (first) { out.append(s); first = false; }
        else       { out.append(del + s); }
    }
    return out;
}

std::string make_sized_line(std::string s,int len) {
    return s.substr(0,len) + "\n";
}

std::string concat_paths(std::vector<std::string> v) {
    std::filesystem::path p = "";
    for(auto s : v) {
        if(s != "") {
            p /= s;
        }
    }

    return p.string();
}

std::string random_alphaNumerical(int length) {
    std::string s;
    for(int i = 0; i < length;i++) {
        int r = rand() % 62; // 0-9 + A-Z + a-z = 62
        if(r <= 9) { 
            s.push_back(r+48); // 48 in ascii equals 0
        } else if(r <= 35) {
            s.push_back(r - 10 + 65); // 65 == A, r == 10 ==> A
        }
        else {
            s.push_back(r - 36 + 97); // 97 == a, r == 36 ==> a
        }
    }

    return s;
}

std::string bytes_to_hex(const std::vector<char> &bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for(auto &b : bytes) {
        ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(b)); 
    };

    return ss.str();
}

std::string bytes_to_hex(const std::deque<char> &bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for(auto &b : bytes) {
        ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(b)); 
    };

    return ss.str();
}

std::vector<char> int_to_bytes(int n)  {
    std::vector<char> v;
    v.push_back(n >> 24);
    v.push_back(n >> 16);
    v.push_back(n >> 8);
    v.push_back(n);

    return v;
}

int bytes_to_int(std::deque<char> &d) {
    if (d.size() < 1) { return {}; }

    int n = 0;
    int size = d.size() > 4 ? 4 : d.size(); //number of bytes in an int
    for(int i = 0; i < size; i++) {
        n |= (unsigned char)d.front() << (size*8 - (i+1) * 8);
        d.pop_front();
    }

    return n;
}

std::vector<char> bitfield_to_bytes(const std::vector<bool> &bits) {
    std::vector<char> v;
    char c = 0;
    int i = 0;
    while(i < bits.size()) {
        c |= bits[i] << (7 - (i % 8));
        i++;

        if(i % 8 == 0) {
            v.push_back(c);
            c = 0;
        }
    }

    //If we did not get a multiple of 8 as input then
    //push the last byte padded with zeros on the right
    if (i % 8 != 0) {
        v.push_back(c);
    }

    return v;
}

std::vector<bool> bytes_to_bitfield(int len,std::deque<char> &bytes) {
    std::vector<bool> v;
    for(int i = 0; i < len; i++) {
        char c = bytes[i];

        for(int j = 0; j < 8; j++) { v.push_back(c >> (7 - j)); }
    }

    return v;
}

std::vector<bool> bytes_to_bitfield(int len,std::vector<char> &bytes) {
    std::vector<bool> v;
    for(int i = 0; i < len; i++) {
        char c = bytes[i];

        for(int j = 0; j < 8; j++) { v.push_back(c >> (7 - j)); }
    }

    return v;
}

std::wstring pp_bytes(long long bytes) {
    std::list<std::wstring> units = { L"B",L"KB",L"MB",L"GB"};
    long double d = bytes;
    auto u = units.front();
    units.pop_front();
    while (d > 1000 && units.size() > 0) {
        d = d / 1000;
        u = units.front();
        units.pop_front();
    }

    auto nat = std::floor(d); // xx.yyzz -> xx
    auto dec = d - std::floor(d); // xx.yyzz - xx -> 0.yyzz
    auto dec_rounded = std::floor(dec*100); // 0.yyzz -> yy

    return std::to_wstring((long long)nat) + L"." + std::to_wstring((long long)dec_rounded) + L" " + u;
}

std::wstring pp_bytes_per_second(long long bytes) {
    return pp_bytes(bytes) + L"/s";
}

std::wstring make_wide(const std::string &s) {
    return std::wstring(s.begin(),s.end());
}

std::string unwide(const std::wstring &ws) {
    return std::string(ws.begin(),ws.end());
}