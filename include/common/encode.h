#pragma once

#include <curl/curl.h>
#include <openssl/sha.h>
#include <memory>
#include <neither/maybe.hpp>
#include <vector>

using namespace std;

vector<unsigned char> sha1_encode(string bytes);

neither::Maybe<string> url_encode (const vector<unsigned char> &bytes);