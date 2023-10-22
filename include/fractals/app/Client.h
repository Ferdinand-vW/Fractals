#pragma once

#include <array>

namespace fractals::app {

    /**
    Generate unique id to be used for peer identification by the BT protcol
    */
    std::array<char, 20> generateAppId();

}