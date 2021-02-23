
#include "common/utils.h"

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