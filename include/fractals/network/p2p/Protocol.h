#pragma once 

#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/persist/PersistEventQueue.h"
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
                    , const std::string& infoHash
                    , PeerService& peerService
                    , persist::PersistEventQueue::LeftEndPoint persistQueue
                    , disk::DiskEventQueue& diskQueue
                    , PieceStateManager& pieceRepository);

            ProtocolState onMessage(HandShake&& hs);
            ProtocolState onMessage(KeepAlive&& hs);
            ProtocolState onMessage(Choke&& hs);
            ProtocolState onMessage(UnChoke&& hs);
            ProtocolState onMessage(Interested&& hs);
            ProtocolState onMessage(NotInterested&& hs);
            ProtocolState onMessage(Have&& hs);
            ProtocolState onMessage(Bitfield&& hs);
            ProtocolState onMessage(Request&& hs);
            ProtocolState onMessage(Piece&& hs);
            ProtocolState onMessage(Cancel&& hs);
            ProtocolState onMessage(Port&& hs);

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
            const std::string& infoHash;
            std::unordered_set<uint32_t> availablePieces;
            PeerService& peerService;
            persist::PersistEventQueue::LeftEndPoint persistQueue;
            disk::DiskEventQueue& diskQueue;
            PieceStateManager& pieceRepository;
    };
}