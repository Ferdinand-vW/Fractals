#pragma once

namespace fractals::network::p2p
{
    class BitTorrentManager
    {
        public:
            BitTorrentManager(BitTorrentMsgQueue& sendQueue
                             ,persist::StorageEventQueue& storageQueue
                             ,disk::DiskEventQueue& diskQueue
                             ,http::TrackerService& trackerService);
    };
}