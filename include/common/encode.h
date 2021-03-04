#pragma once

#include <curl/curl.h>
#include <openssl/sha.h>
#include <memory>
#include <neither/maybe.hpp>
#include <vector>

using namespace std;

vector<char> sha1_encode(string bytes);

std::string url_encode (const vector<char> &bytes);