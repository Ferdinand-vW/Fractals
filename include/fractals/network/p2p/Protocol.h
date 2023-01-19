#pragma once 

#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/persist/StorageEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"

#include <cstdint>
#include <unordered_set>

namespace fractals::network::p2p
{   
    class Protocol
    {
        public:
            Protocol(http::PeerId peer
                    , BitTorrentMsgQueue& sendQueue
                    , persist::StorageEventQueue& storageQueue
                    , PieceStateManager& pieceRepository);

            void onMessage(const HandShake& hs);
            void onMessage(const KeepAlive& hs);
            void onMessage(const Choke& hs);
            void onMessage(const UnChoke& hs);
            void onMessage(const Interested& hs);
            void onMessage(const NotInterested& hs);
            void onMessage(const Have& hs);
            void onMessage(const Bitfield& hs);
            void onMessage(const Request& hs);
            void onMessage(const Piece& hs);
            void onMessage(const Cancel& hs);
            void onMessage(const Port& hs);

        private:
            void sendInterested();
            void requestNextPiece();

        private:
            std::unordered_set<uint32_t> mAvailablePieces;
            bool mAmChoking{true};
            bool mAmInterested{false};
            bool mPeerChoking{true};
            bool mPeerInterested{false};

            http::PeerId peer;
            BitTorrentMsgQueue& sendQueue;
            persist::StorageEventQueue& storageQueue;
            PieceStateManager& pieceRepository;
    };
}