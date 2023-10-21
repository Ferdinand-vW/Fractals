#include "fractals/common/Tagged.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/network/p2p/Protocol.h"
#include <boost/mp11/algorithm.hpp>
#include <chrono>
#include <cstdint>
#include <new>

namespace fractals::network::p2p
{
template <typename PeerServiceT>
Protocol<PeerServiceT>::Protocol(const common::AppId &appId, http::PeerId peer,
                                 const common::InfoHash &infoHash, PeerServiceT &peerService,
                                 disk::DiskEventQueue::LeftEndPoint diskQueue,
                                 PieceStateManager &pieceRepository)
    : appId(appId), peer(peer), infoHash(infoHash), peerService(peerService),
       diskQueue(diskQueue), pieceRepository(pieceRepository)
{
}

template <typename PeerServiceT>
bool Protocol<PeerServiceT>::sendHandShake(std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Write HandShake", peer.toString(), infoHash);
    static const std::string protocol("BitTorrent protocol");
    static const std::array<char, 8> reserved{0, 0, 0, 0, 0, 0, 0, 0};
    return peerService.write(
        this->peer, HandShake{protocol, reserved, infoHash.underlying, appId.underlying}, now);
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const HandShake &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received HandShake", peer.toString(), infoHash);
    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const KeepAlive &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received KeepAlive", peer.toString(), infoHash);
    spdlog::info("Protocol({}, {}). Write KeepAlive", peer.toString(), infoHash);
    peerService.write(peer, KeepAlive{}, now);
    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const Choke &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Choke", peer.toString(), infoHash);
    spdlog::info("Protocol({}). Received Choke", infoHash);
    mPeerChoking = true;
    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const UnChoke &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Unchoke", peer.toString(), infoHash);
    if (mPeerChoking)
    {
        mPeerChoking = false;

        return requestNextPiece(now);
    }

    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const Interested &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Interested", peer.toString(), infoHash);
    mPeerInterested = true;

    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const NotInterested &hs,
                                                std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received NotInterested", peer.toString(), infoHash);
    mPeerInterested = false;

    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const Have &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Have", peer.toString(), infoHash);
    if (!pieceRepository.isCompleted(hs.getPieceIndex()))
    {
        availablePieces.emplace(hs.getPieceIndex());
        sendInterested(now);
    }

    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const Bitfield &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Bitfield", peer.toString(), infoHash);
    const auto vb = common::bytes_to_bitfield(hs.getBitfield().size(), hs.getBitfield());
    std::stringstream ss;
    for (auto i : vb)
    {
        ss << std::to_string(i);
    }

    uint32_t pieceIndex = 0;
    for (bool b : vb)
    {
        if (b)
        {
            availablePieces.emplace(pieceIndex);
        }

        pieceIndex++;
    }

    sendInterested(now);

    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const Request &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Request", peer.toString(), infoHash);
    // TODO
    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const Piece &p, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Piece", peer.toString(), infoHash);
    PieceState *ps = pieceRepository.getPieceState(p.getPieceIndex());
    if (ps == nullptr)
    {
        if (pieceRepository.isCompleted(p.getPieceIndex()))
        {
            spdlog::info("Protocol::onMessage(piece). Piece already completed {}",
                         p.getPieceIndex());
            return ProtocolState::OPEN;
        }

        spdlog::error("Protocol::onMessage(Piece). Unable to find PieceState for {}",
                      p.getPieceIndex());
        return ProtocolState::ERROR;
    }

    // Sanity check that we receive the block of data that we requested
    if (p.getPieceBegin() == ps->getNextBeginIndex())
    {
        ps->addBlock(p.getBlock());

        if (ps->isComplete())
        {
            // Ensure integrity of data
            if (pieceRepository.hashCheck(ps->getPieceIndex(), ps->getBuffer()))
            {
                // Update local state
                diskQueue.push(disk::WriteData{infoHash, p.getPieceIndex(),
                                               std::move(ps->extractData()), ps->getOffset()});

                // Update in-memory state
                pieceRepository.makeCompleted(p.getPieceIndex());
                availablePieces.erase(p.getPieceIndex());
            }
            else
            {
                spdlog::error("Protocol::onMessage(Piece). HashCheckFail");
                ps->clear();
                return ProtocolState::HASH_CHECK_FAIL;
            }
        }
    }
    else
    {
        spdlog::warn("Protocol::onMessage(Piece). Already received payload for {}",
                     p.getPieceIndex());
    }

    return requestNextPiece(now);
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const Cancel &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Cancel", peer.toString(), infoHash);
    return ProtocolState::CLOSED;
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::onMessage(const Port &hs, std::chrono::nanoseconds now)
{
    spdlog::info("Protocol({}, {}). Received Port", peer.toString(), infoHash);
    return ProtocolState::OPEN;
}

template <typename PeerServiceT>
void Protocol<PeerServiceT>::sendInterested(std::chrono::nanoseconds now)
{
    if (!mAmInterested)
    {
        spdlog::info("Protocol({}, {}). Send Interested", peer.toString(), infoHash);
        peerService.write(this->peer, Interested{}, now);
        mAmInterested = true;
    }
}

template <typename PeerServiceT>
ProtocolState Protocol<PeerServiceT>::requestNextPiece(std::chrono::nanoseconds now)
{
    const auto nextPiece = getNextAvailablePiece();
    if (nextPiece != nullptr && pieceRepository.isActive())
    {
        uint32_t standardPacketSize = 1 << 14; // 16 KB
        uint32_t remainingOfPiece = nextPiece->getRemainingSize();
        uint32_t requestSize = std::min(standardPacketSize, remainingOfPiece);

        Request request{nextPiece->getPieceIndex(), nextPiece->getNextBeginIndex(), requestSize};

        if (!mPeerChoking)
        {
            spdlog::info("Protocol({}, {}). Send Piece={}", peer.toString(), infoHash,
                         nextPiece->getPieceIndex());
            peerService.write(this->peer, request, now);
        }

        return ProtocolState::OPEN;
    }

    if (!pieceRepository.isAllComplete() || !pieceRepository.isActive())
    {
        return ProtocolState::CLOSED;
    }
    else
    {
        return ProtocolState::COMPLETE;
    }
}

template <typename PeerServiceT> PieceState *Protocol<PeerServiceT>::getNextAvailablePiece()
{
    return pieceRepository.nextAvailablePiece(availablePieces);
}

template <typename PeerServiceT> const common::InfoHash &Protocol<PeerServiceT>::getInfoHash() const
{
    return infoHash;
}
} // namespace fractals::network::p2p