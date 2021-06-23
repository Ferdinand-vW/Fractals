#pragma once

#include "torrent/PieceData.h"

enum class PieceProgress { Nothing, Requested, Downloaded, Completed };
enum class PeerChange { Added, Removed };
struct PeerWork {
    PieceProgress m_progress;
    PieceData m_data;
};