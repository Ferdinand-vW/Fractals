#pragma once

#include "fractals/common/Tagged.h"
#include <fractals/app/Client.h>

#include <array>

namespace fractals
{

class Fractals
{
  public:
    static const common::AppId& initAppId()
    {
        APPID = app::generate_peerId();
        return APPID;
    }

    static common::AppId APPID;
};

inline common::AppId Fractals::APPID = {};

} // namespace fractals