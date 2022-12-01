#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <list>
#include <cmath> // for floor
#include <math.h>
#include <string>
#include <filesystem>
#include <utility>
#include <fstream>

#include "fractals/common/utils.h"

namespace fractals::common {

    std::string concat(const std::vector<std::string> &v) {
        std::string out("");
        for(auto s : v) {
            out.append(s);
        }
        return out;
    }

    std::string concat(const std::vector<char>& cs) {
        std::string out{""};

        for(char c : cs)
        {
            out.append(std::to_string(c));
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

    std::string intercalate(std::string del, const std::vector<char> &v)
    {
        std::string out{""};
        bool first = true;
        for (auto c : v)
        {
            if (first) { out.append(std::to_string(c)); first = false; }
            else       { out.append(del + std::to_string(c)); }
        }

        return out;
    }

    std::string intercalate(std::string del, const std::deque<char> &v)
    {
        std::string out{""};
        bool first = true;
        for(auto c : v)
        {
            if (first) { out.append(std::to_string(c)); first = false; }
            else       { out.append(del + std::to_string(c)); }
        }

        return out;
    }


    std::string make_sized_line(std::string s,int len) {
        return s.substr(0,len) + "\n";
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

    std::vector<char> hex_to_bytes(const std::string& s)
    {
        std::vector<char> bytes;
        bytes.reserve(std::ceil(s.size() / (double)2));
        for(int i = 0; i+1 < s.size(); i+=2)
        {
            unsigned int c;
            std::stringstream ss;
            ss << std::hex << std::string(s.begin() + i, s.begin() + i + 2);
            ss >> c;

            bytes.emplace_back(c);
        }

        return bytes;
    }

    std::vector<char> int_to_bytes(int n)  {
        std::vector<char> v;
        //int is 4 bytes
        //if int n consist of bytes W X Y Z then converting to char leaves us with Z
        v.push_back(n >> 24); //Move W to 0 0 0 W
        v.push_back(n >> 16); //Move X to 0 0 W X
        v.push_back(n >> 8); //Move Y to 0 W X Y
        v.push_back(n); //Z is already in right location

        return v;
    }

    int bytes_to_int(std::deque<char> &d) {
        if (d.size() < 1) { return {}; }

        int n = 0;
        int size = d.size() > 4 ? 4 : d.size(); //We may have fewer than 4 bytes
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
            d.pop_front();
        }

        return n;
    }

    std::vector<char> bitfield_to_bytes(const std::vector<bool> &bits) {
        std::vector<char> v;
        char c = 0;
        int i = 0;
        while(i < bits.size()) {
            //bits[i] is either 1 or 0. Here we move it to the right bit location in c
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

            //convert each bit in c to a bool and push into the vector
            for(int j = 0; j < 8; j++) { 
                v.push_back(c >> (7 - j)); 
            }
        }

        return v;
    }

    std::vector<bool> bytes_to_bitfield(int len,std::vector<char> &bytes) {
        std::vector<bool> v;
        for(int i = 0; i < len; i++) {
            char c = bytes[i];

            //convert each bit in c to a bool and push into the vector
            for(int j = 0; j < 8; j++) { 
                int mvr = 7 - j;

                unsigned char sig = '\x80';
                unsigned char bitPos = sig >> j; // =100000000 >> mvr

                char pickedBit = c & bitPos;


                v.push_back(pickedBit >> mvr); 
            }
        }

        return v;
    }

    std::wstring pp_bytes(int64_t bytes) {
        std::list<std::wstring> units = { L"B",L"KB",L"MB",L"GB",L"TB",L"PB"};
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

        return std::to_wstring((int64_t)nat) + L"." + std::to_wstring((int64_t)dec_rounded) + L" " + u;
    }

    std::wstring pp_bytes_per_second(int64_t bytes) {
        return pp_bytes(bytes) + L"/s";
    }

    std::wstring pp_time(int64_t seconds) {
        // >= 3 years
        if(seconds >= 279936000) {
            return L"inf";
        }

        //not entirely correct, but we don't need it to be
        std::list<std::pair<std::wstring,int>> units = 
            { {L"m",60}, {L"h",60}, {L"d",24}, {L"m",30}, {L"y",12} };

        std::list<std::pair<std::wstring,int>> measurements;

        std::pair<std::wstring,int64_t> prev(L"s",seconds);
        for(auto &u : units) {
            auto nat = prev.second / u.second; //number of minutes/hours/days/..
            auto prev_units = prev.second - nat*u.second;
            if(nat > 0) {
                measurements.push_front({prev.first,prev_units});
                prev = std::make_pair(u.first,nat);
            } else {
                break;
            }
        }
        measurements.push_front(prev);

        std::wstring ws;
        for(auto &m : measurements) {
            ws = ws + std::to_wstring(m.second) + m.first;
        }

        return ws;
    }

    std::wstring make_wide(const std::string &s) {
        return std::wstring(s.begin(),s.end());
    }

    std::string unwide(const std::wstring &ws) {
        return std::string(ws.begin(),ws.end());
    }

    void print_err(std::string &&s) {
        std::ofstream ofs("err2.txt");
        ofs << s;
        ofs.close();
    }

}