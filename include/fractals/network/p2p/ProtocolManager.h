#pragma once

#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/persist/StorageEventQueue.h"

namespace fractals::network::p2p
{
    class ProtocolManager
    {
        public:
            // s
            ProtocolManager( BitTorrentMsgQueue& sendQueue
                           , persist::StorageEventQueue& storageQueue
                           , disk::DiskEventQueue& diskQueue
                           , http::TrackerRequestQueue& trackerQueue);

            BitTorrentMsgQueue& sendQueue;
            persist::StorageEventQueue& storageQueue;
            disk::DiskEventQueue& diskQueue;

            PieceStateManager pieceMan;
    };
}