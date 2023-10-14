#pragma once

#include <fractals/app/Client.h>

#include <array>

namespace fractals
{

class Fractals
{
  public:
    static void initAppId()
    {
        APPID = app::generate_peerId();
    }

    static std::array<char, 20> APPID;
};

inline std::array<char, 20> Fractals::APPID = {};

} // namespace fractals