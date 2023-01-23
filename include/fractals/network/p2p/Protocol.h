#pragma once 

#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/persist/StorageEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/ProtocolState.h"

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
                    , disk::DiskEventQueue& diskQueue
                    , PieceStateManager& pieceRepository);

            ProtocolState onMessage(const HandShake& hs);
            ProtocolState onMessage(const KeepAlive& hs);
            ProtocolState onMessage(const Choke& hs);
            ProtocolState onMessage(const UnChoke& hs);
            ProtocolState onMessage(const Interested& hs);
            ProtocolState onMessage(const NotInterested& hs);
            ProtocolState onMessage(const Have& hs);
            ProtocolState onMessage(const Bitfield& hs);
            ProtocolState onMessage(const Request& hs);
            ProtocolState onMessage(Piece&& hs);
            ProtocolState onMessage(const Cancel& hs);
            ProtocolState onMessage(const Port& hs);

        private:
            void sendInterested();
            ProtocolState requestNextPiece();
            PieceState* getNextAvailablePiece();

        private:
            bool mAmChoking{true};
            bool mAmInterested{false};
            bool mPeerChoking{true};
            bool mPeerInterested{false};

            http::PeerId peer;
            std::unordered_set<uint32_t> availablePieces;
            BitTorrentMsgQueue& sendQueue;
            persist::StorageEventQueue& storageQueue;
            disk::DiskEventQueue& diskQueue;
            PieceStateManager& pieceRepository;
    };
}