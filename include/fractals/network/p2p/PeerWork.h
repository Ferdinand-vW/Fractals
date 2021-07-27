#pragma once

#include "fractals/torrent/PieceData.h"

namespace fractals::network::p2p {

    enum class PieceProgress { Nothing, Requested, Downloaded, Completed };
    enum class PeerChange { Added, Removed };

    /**
    A Peer can be assigned one piece at a time.
    The downloaded data for this piece is contained in @m_data.
    What the peer is currently doing is represented by @m_progress.
    */
    struct PeerWork {
        PieceProgress m_progress;
        torrent::PieceData m_data;
    };

}