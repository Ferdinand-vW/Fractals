#pragma once

#include <vector>

namespace fractals::app {

    /**
    Generate unique id to be used for peer identification by the BT protcol
    */
    std::vector<char> generate_peerId();

}