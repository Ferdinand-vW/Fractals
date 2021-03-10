#pragma once

#include <curl/curl.h>
#include <openssl/sha.h>
#include <memory>
#include <neither/maybe.hpp>
#include <vector>

std::vector<char> sha1_encode(std::string bytes);

std::string url_encode (const std::vector<char> &bytes);