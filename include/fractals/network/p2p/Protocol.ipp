#include "fractals/network/p2p/Protocol.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/PersistEventQueue.h"
#include <cstdint>
#include <new>

namespace fractals::network::p2p
{
template <typename PeerServiceT>
Protocol<PeerServiceT>::Protocol(http::PeerId peer, const std::string &infoHash, PeerServiceT &peerService,
                                 persist::PersistEventQueue::LeftEndPoint persistQueue, disk::DiskEventQueue &diskQueue,
                                 PieceStateManager &pieceRepository)
    : peer(peer), infoHash(infoHash), peerService(peerService), persistQueue(persistQueue), diskQueue(diskQueue),
      pieceRepository(pieceRepository)
{
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(HandShake &&hs)
{
    // TODO
    return ProtocolState::OPEN;
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(KeepAlive &&hs)
{
    return ProtocolState::OPEN;
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(Choke &&hs)
{
    mPeerChoking = true;
    return ProtocolState::OPEN;
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(UnChoke &&hs)
{
    mPeerChoking = false;

    return requestNextPiece();
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(Interested &&hs)
{
    mPeerInterested = true;

    return ProtocolState::OPEN;
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(NotInterested &&hs)
{
    mPeerInterested = false;

    return ProtocolState::OPEN;
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(Have &&hs)
{
    if (!pieceRepository.isCompleted(hs.getPieceIndex()))
    {
        availablePieces.emplace(hs.getPieceIndex());

        sendInterested();
    }

    return ProtocolState::OPEN;
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(Bitfield &&hs)
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

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(Request &&hs)
{
    // TODO
    return ProtocolState::OPEN;
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(Piece &&p)
{
    PieceState *ps = pieceRepository.getPieceState(p.getPieceIndex());
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
                ps->clear();
                return ProtocolState::HASH_CHECK_FAIL;
            }
        }
    }

    return requestNextPiece();
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(Cancel &&hs)
{
    return ProtocolState::CLOSED;
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::onMessage(Port &&hs)
{
    return ProtocolState::OPEN;
}

template <typename PeerServiceT> void Protocol<PeerServiceT>::sendInterested()
{
    if (!mAmInterested)
    {
        peerService.write(this->peer, Interested{});
        mAmInterested = true;
    }
}

template <typename PeerServiceT> ProtocolState Protocol<PeerServiceT>::requestNextPiece()
{
    const auto nextPiece = getNextAvailablePiece();
    if (nextPiece)
    {
        uint32_t standardPacketSize = 1 << 14; // 16 KB
        uint32_t remainingOfPiece = nextPiece->getRemainingSize();
        uint32_t requestSize = std::min(standardPacketSize, remainingOfPiece);

        Request request{nextPiece->getPieceIndex(), nextPiece->getNextBeginIndex(), requestSize};

        if (!mPeerChoking)
        {
            peerService.write(this->peer, request);
        }

        return ProtocolState::OPEN;
    }

    return ProtocolState::CLOSED;
}

template <typename PeerServiceT> PieceState *Protocol<PeerServiceT>::getNextAvailablePiece()
{
    auto it = availablePieces.begin();

    if (it != availablePieces.end())
    {
        return pieceRepository.getPieceState(*it);
    }

    return nullptr;
}
} // namespace fractals::network::p2p