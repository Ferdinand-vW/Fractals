#include <algorithm>
#include <cmath> // for floor
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <list>
#include <math.h>
#include <string>
#include <utility>

#include <fractals/common/utils.h>

namespace fractals::common
{

std::string concat(const std::vector<std::string> &v)
{
    std::string out("");
    for (auto s : v)
    {
        out.append(s);
    }
    return out;
}

std::string concat(const std::vector<char> &cs)
{
    std::string out{""};

    for (char c : cs)
    {
        out.append(std::to_string(c));
    }

    return out;
}

std::string intercalate(std::string del, const std::vector<std::string> &v)
{
    std::string out("");
    bool first = true;
    for (auto s : v)
    {
        if (first)
        {
            out.append(s);
            first = false;
        }
        else
        {
            out.append(del + s);
        }
    }
    return out;
}

std::string intercalate(std::string del, const std::vector<char> &v)
{
    std::string out{""};
    bool first = true;
    for (auto c : v)
    {
        if (first)
        {
            out.append(std::to_string(c));
            first = false;
        }
        else
        {
            out.append(del + std::to_string(c));
        }
    }

    return out;
}

std::string intercalate(std::string del, const std::deque<char> &v)
{
    std::string out{""};
    bool first = true;
    for (auto c : v)
    {
        if (first)
        {
            out.append(std::to_string(c));
            first = false;
        }
        else
        {
            out.append(del + std::to_string(c));
        }
    }

    return out;
}

std::string makeSizedLine(std::string s, int len)
{
    return s.substr(0, len) + "\n";
}

std::string randomAlphaNumerical(int length)
{
    std::string s;
    for (int i = 0; i < length; i++)
    {
        int r = rand() % 62; // 0-9 + A-Z + a-z = 62
        if (r <= 9)
        {
            s.push_back(r + 48); // 48 in ascii equals 0
        }
        else if (r <= 35)
        {
            s.push_back(r - 10 + 65); // 65 == A, r == 10 ==> A
        }
        else
        {
            s.push_back(r - 36 + 97); // 97 == a, r == 36 ==> a
        }
    }

    return s;
}

std::string bytesToHex(std::string_view view)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto &b : view)
    {
        ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(b));
    };

    return ss.str();
}

std::string bytesToHex(const std::vector<char> &bytes)
{
    return bytesToHex(std::string_view(bytes.begin(), bytes.end()));
}

std::string bytesToHex(const std::deque<char> &bytes)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto &b : bytes)
    {
        ss << std::setw(2) << static_cast<int>(static_cast<unsigned char>(b));
    };

    return ss.str();
}

std::vector<char> hexToBytes(const std::string &s)
{
    std::vector<char> bytes;
    bytes.reserve(std::ceil(s.size() / (double)2));
    for (int i = 0; i + 1 < s.size(); i += 2)
    {
        unsigned int c;
        std::stringstream ss;
        ss << std::hex << std::string(s.begin() + i, s.begin() + i + 2);
        ss >> c;

        bytes.emplace_back(c);
    }

    return bytes;
}

int bytesToInt(std::deque<char> &d)
{
    if (d.size() < 1)
    {
        return {};
    }

    int n = 0;
    int size = d.size() > 4 ? 4 : d.size(); // We may have fewer than 4 bytes
    for (int i = 0; i < size; i++)
    {
        // Example:
        // Assume size = 3 (X Y Z) and start with 0 0 0 0, then
        // byte X must be moved 16bits left: 3*8 - (0+1)*8 = 16, 0 X 0 0
        // byte Y must be moved 8bits left: 3*8 - (1+1)*8 = 8, 0 X Y 0
        // byte Z must be moved 0bits left: 3*8 - (2+1)*8 = 0, 0 X Y Z
        //  note that the | operator does or operations between bytes:
        //  0 X 0 0
        //  0 0 Y 0
        //  ------- or (|)
        //  0 X Y 0
        n |= (unsigned char)d.front() << (size * 8 - (i + 1) * 8);
        d.pop_front();
    }

    return n;
}

std::vector<char> bitfieldToBytes(const std::vector<bool> &bits)
{
    std::vector<char> v;
    char c = 0;
    int i = 0;
    while (i < bits.size())
    {
        // bits[i] is either 1 or 0. Here we move it to the right bit location in c
        c |= bits[i] << (7 - (i % 8));
        i++;

        if (i % 8 == 0)
        {
            v.push_back(c);
            c = 0;
        }
    }

    // If we did not get a multiple of 8 as input then
    // push the last byte padded with zeros on the right
    if (i % 8 != 0)
    {
        v.push_back(c);
    }

    return v;
}

std::vector<bool> bytesToBitfield(int len, std::deque<char> &bytes)
{
    std::vector<bool> v;
    for (int i = 0; i < len; i++)
    {
        char c = bytes[i];

        // convert each bit in c to a bool and push into the vector
        for (int j = 0; j < 8; j++)
        {
            v.push_back(c >> (7 - j));
        }
    }

    return v;
}

std::vector<bool> bytesToBitfield(int len, std::string_view bytes)
{
    std::vector<bool> v;
    for (int i = 0; i < len; i++)
    {
        char c = bytes[i];

        // convert each bit in c to a bool and push into the vector
        for (int j = 0; j < 8; j++)
        {
            int mvr = 7 - j;

            unsigned char sig = '\x80';
            unsigned char bitPos = sig >> j; // =100000000 >> mvr

            char pickedBit = c & bitPos;

            v.push_back(pickedBit >> mvr);
        }
    }

    return v;
}

std::string ppBytes(int64_t bytes)
{
    std::list<std::string> units = {"B", "KB", "MB", "GB", "TB", "PB"};
    long double d = bytes;
    auto u = units.front();
    units.pop_front();
    while (d > 1000 && units.size() > 0)
    {
        d = d / 1000;
        u = units.front();
        units.pop_front();
    }

    auto nat = std::floor(d);                 // xx.yyzz -> xx
    auto dec = d - std::floor(d);             // xx.yyzz - xx -> 0.yyzz
    auto dec_rounded = std::floor(dec * 100); // 0.yyzz -> yy

    return std::to_string((int64_t)nat) + "." + std::to_string((int64_t)dec_rounded) + " " + u;
}

std::string ppBytesPerSecond(int64_t bytes)
{
    return ppBytes(bytes) + "/s";
}

std::string ppTime(int64_t seconds)
{
    // >= 3 years
    if (seconds >= 279936000)
    {
        return "inf";
    }

    // not entirely correct, but we don't need it to be
    std::list<std::pair<std::string, int>> units = {
        {"m", 60}, {"h", 60}, {"d", 24}, {"m", 30}, {"y", 12}};

    std::list<std::pair<std::string, int>> measurements;

    std::pair<std::string, int64_t> prev("s", seconds);
    for (auto &u : units)
    {
        auto nat = prev.second / u.second; // number of minutes/hours/days/..
        auto prevUnits = prev.second - nat * u.second;
        if (nat > 0)
        {
            measurements.push_front({prev.first, prevUnits});
            prev = std::make_pair(u.first, nat);
        }
        else
        {
            break;
        }
    }
    measurements.push_front(prev);

    std::string ws;
    for (auto &m : measurements)
    {
        ws = ws + std::to_string(m.second) + m.first;
    }

    return ws;
}

void printErr(std::string &&s)
{
    std::ofstream ofs("err2.txt");
    ofs << s;
    ofs.close();
}

} // namespace fractals::common