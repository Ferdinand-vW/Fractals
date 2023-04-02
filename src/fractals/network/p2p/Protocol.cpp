#include "fractals/network/p2p/Protocol.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/PersistEventQueue.h"
#include <cstdint>
#include <new>

namespace fractals::network::p2p
{
    Protocol::Protocol(http::PeerId peer
            , const std::string& infoHash
            , BitTorrentMsgQueue& sendQueue
            , persist::PersistEventQueue::LeftEndPoint persistQueue
            , disk::DiskEventQueue& diskQueue
            , PieceStateManager& pieceRepository)
            : peer(peer)
            , infoHash(infoHash)
            , sendQueue(sendQueue)
            , persistQueue(persistQueue)
            , diskQueue(diskQueue)
            , pieceRepository(pieceRepository)
            {}


    ProtocolState Protocol::onMessage(HandShake&& hs)
    {
        //TODO
        return ProtocolState::OPEN;
    }

    ProtocolState Protocol::onMessage(KeepAlive&& hs)
    {
        return ProtocolState::OPEN;
    }

    ProtocolState Protocol::onMessage(Choke&& hs)
    {
        mPeerChoking = true;
        return ProtocolState::OPEN;
    }

    ProtocolState Protocol::onMessage(UnChoke&& hs)
    {
        mPeerChoking = false;

        return requestNextPiece();
    }

    ProtocolState Protocol::onMessage(Interested&& hs)
    {
        mPeerInterested = true;

        return ProtocolState::OPEN;
    }

    ProtocolState Protocol::onMessage(NotInterested&& hs)
    {
        mPeerInterested = false;

        return ProtocolState::OPEN;
    }

    ProtocolState Protocol::onMessage(Have&& hs)
    {
        if (!pieceRepository.isCompleted(hs.getPieceIndex()))
        {
            availablePieces.emplace(hs.getPieceIndex());

            sendInterested();
        }

        return ProtocolState::OPEN;
    }

    ProtocolState Protocol::onMessage(Bitfield&& hs)
    {
        // hs.getBitfield()
        const auto vb = common::bytes_to_bitfield(hs.getBitfield().size(), hs.getBitfield());
        uint32_t pieceIndex = 0;
        for (bool b : vb)
        {
            if (b) 
            {
                availablePieces.emplace(pieceIndex);
            }

            pieceIndex++;
        }

        sendInterested();

        return ProtocolState::OPEN;
    }

    ProtocolState Protocol::onMessage(Request&& hs)
    {
        //TODO
        return ProtocolState::OPEN;
    }

    ProtocolState Protocol::onMessage(Piece&& p)
    {
        PieceState* ps = pieceRepository.getPieceState(p.getPieceIndex());
        if (!ps)
        {
            return ProtocolState::ERROR;
        }

        // Sanity check that we receive the block of data that we requested
        if (p.getPieceBegin() == ps->getNextBeginIndex())
        {
            ps->addBlock(p.extractBlock());

            if (ps->isComplete())
            {
                // Ensure integrity of data
                if (pieceRepository.hashCheck(ps->getPieceIndex(), ps->getBuffer()))
                {
                    // Update local state
                    persistQueue.push(persist::AddPiece{infoHash, p.getPieceIndex()});
                    diskQueue.push(disk::WriteData{p.getPieceIndex(), std::move(ps->extractData())});

                    // Update in-memory state
                    pieceRepository.makeCompleted(p.getPieceIndex());
                    availablePieces.erase(p.getPieceIndex());
                }
                else
                {
                    return ProtocolState::HASH_CHECK_FAIL;
                }
            }
        }

        return requestNextPiece();
    }

    ProtocolState Protocol::onMessage(Cancel&& hs)
    {
        return ProtocolState::CLOSED;
    }

    ProtocolState Protocol::onMessage(Port&& hs)
    {
        return ProtocolState::OPEN;
    }

    void Protocol::sendInterested()
    {
        if (!mAmInterested)
        {
            sendQueue.push(Interested{});
            mAmInterested = true;
        }
    }

    ProtocolState Protocol::requestNextPiece()
    {
        const auto nextPiece = getNextAvailablePiece();
        if (nextPiece)
        {
            uint32_t standardPacketSize = 1 << 14; // 16 KB
            uint32_t remainingOfPiece = nextPiece->getRemainingSize();
            uint32_t requestSize = std::min(standardPacketSize, remainingOfPiece);

            Request request{nextPiece->getPieceIndex(), nextPiece->getNextBeginIndex(), requestSize};

            sendQueue.push(request);

            return ProtocolState::OPEN;
        }

        return ProtocolState::CLOSED;
    }

    PieceState* Protocol::getNextAvailablePiece()
    {
        auto it = availablePieces.begin();

        if (it != availablePieces.end())
        {
            return pieceRepository.getPieceState(*it);
        }

        return nullptr;
    }
}