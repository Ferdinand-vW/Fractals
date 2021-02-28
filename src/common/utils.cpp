
#include "common/utils.h"
#include <cstdlib>
#include <string>

std::string str_concat_vector(std::vector<std::string> v) {
    std::string out("");
    for(auto s : v) {
        out.append(s);
    }
    return out;
}

std::string intercalate(std::string del,std::vector<std::string> v) {
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

std::string random_alphaNumerical(int length) {
    std::string s;
    for(int i = 0; i < length;i++) {
        int r = rand() % 62; // 0-9 + A-Z + a-Z = 62
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

std::vector<char> int_to_bytes(int n)  {
    std::vector<char> v;
    v.push_back(n >> 24);
    v.push_back(n >> 16);
    v.push_back(n >> 8);
    v.push_back(n);

    return v;
}

std::vector<char> bitfield_to_bytes(std::vector<bool> bits) {
    std::vector<char> v;
    char c = 0;
    int i = 0;
    while(i < bits.size()) {
        if(i % 8 == 0) {
            v.push_back(c);
            c = 0;
        }

        c |= bits[i] << (7 - (i % 8));
        i++;
    }

    //If we did not get a multiple of 8 as input then
    //push the last byte padded with zeros on the right
    if (i % 8 != 0) {
        v.push_back(c);
    }

    return v;
}

/*
s = 7 i = 0
s = 6 i = 1
s = 5 i = 2
s = 4 i = 3
s = 3 i = 4
s = 2 i = 5
s = 1 i = 6
s = 0 i = 7
s = 7 i = 0
*/