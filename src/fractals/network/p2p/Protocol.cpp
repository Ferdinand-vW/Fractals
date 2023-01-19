#include "fractals/network/p2p/Protocol.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/StorageEventQueue.h"
#include <cstdint>

namespace fractals::network::p2p
{
    Protocol::Protocol(http::PeerId peer
            , PeerEventQueue& sendQueue
            , persist::StorageEventQueue& storageQueue)
            , PieceStateManager& pieceRepository
            : peer(peer)
            , sendQueue(sendQueue)
            , storageQueue(storageQueue)
            , pieceRepository(pieceRepository)
            {}


    void Protocol::onMessage(const HandShake& hs)
    {
        //TODO
    }

    void Protocol::onMessage(const KeepAlive& hs)
    {

    }

    void Protocol::onMessage(const Choke& hs)
    {
        mPeerChoking = true;
    }

    void Protocol::onMessage(const UnChoke& hs)
    {
        mPeerChoking = false;

        requestNextPiece();
    }

    void Protocol::onMessage(const Interested& hs)
    {
        mPeerInterested = true;
    }

    void Protocol::onMessage(const NotInterested& hs)
    {
        mPeerInterested = false;
    }

    void Protocol::onMessage(const Have& hs)
    {
        // Check if we already have piece
        mAvailablePieces.emplace(hs.getPieceIndex());

        sendInterested();
    }

    void Protocol::onMessage(const Bitfield& hs)
    {
        mAvailablePieces.insert(hs.getBitfield().begin(), hs.getBitfield().end());
        sendInterested();
    }

    void Protocol::onMessage(const Request& hs)
    {
        //TODO
    }

    void Protocol::onMessage(const Piece& p)
    {
        PieceState* ps = pieceRepository.getPieceState(p.getPieceIndex());
        if (!ps)
        {
            return;
        }

        // Sanity check that we receive the block of data that we requested
        if (p.getBeginIndex() == ps->getNextBeginIndex())
        {
            ps->addBlock(p.getBlock());

            if (ps->isComplete())
            {
                // Update local state
                storageQueue.push(persist::AddPieces{p.getPieceIndex(), ps.getPieceData()});

                // Update in-memory state
                mAvailablePieces.erase(p.getPieceIndex());
            }
        }

        requestNextPiece();
    }

    void Protocol::onMessage(const Cancel& hs)
    {

    }

    void Protocol::onMessage(const Port& hs)
    {

    }

    void Protocol::sendInterested()
    {
        if (!mAmInterested)
        {
            sendQueue.push(Interested{});
            mAmInterested = true;
        }
    }

    void Protocol::requestNextPiece()
    {
        if (!mAvailablePieces.empty())
        {
            uint32_t pieceIndex = *mAvailablePieces.begin();
            const auto* pieceState = pieceRepository.getPieceState(pieceIndex);
            if (!pieceState) { return; }
            
            uint64_t standardPacketSize = 1 << 14; // 16 KB
            uint64_t remainingOfPiece = pieceState.getRemainingSize();
            uint64_t requestSize = std::min(standardPacketSize, remainingOfPiece);

            Request request{pieceIndex, pieceState.getNextBeginIndex(), requestSize};

            sendQueue.push(request);
        }
    }
}