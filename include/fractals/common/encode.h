#pragma once

#include <string>
#include <vector>

namespace fractals::common {

    std::vector<char> sha1_encode(std::string bytes);

    std::string url_encode (const std::vector<char> &bytes);

}