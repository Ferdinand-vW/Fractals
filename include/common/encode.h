#pragma once

#include <string>
#include <vector>

std::vector<char> sha1_encode(std::string bytes);

std::string url_encode (const std::vector<char> &bytes);