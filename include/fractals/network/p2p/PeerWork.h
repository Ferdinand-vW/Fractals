#pragma once

#include "fractals/torrent/PieceData.h"

namespace fractals::network::p2p {

    enum class PieceProgress { Nothing, Requested, Downloaded, Completed };
    enum class PeerChange { Added, Removed };
    struct PeerWork {
        PieceProgress m_progress;
        torrent::PieceData m_data;
    };

}