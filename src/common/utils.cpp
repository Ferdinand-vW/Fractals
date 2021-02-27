
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