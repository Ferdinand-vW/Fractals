#pragma once

#include <fractals/common/Tagged.h>
#include <fractals/disk/DiskEventQueue.h>
#include <fractals/network/http/Peer.h>
#include <fractals/network/p2p/BitTorrentMsg.h>
#include <fractals/network/p2p/BufferedQueueManager.h>
#include <fractals/network/p2p/PeerService.h>
#include <fractals/network/p2p/PieceStateManager.h>
#include <fractals/network/p2p/ProtocolState.h>
#include <fractals/persist/PersistEventQueue.h>

#include <chrono>
#include <cstdint>
#include <unordered_set>

namespace fractals::network::p2p
{
template <typename PeerServiceT> class Protocol
{
  public:
    Protocol(const common::AppId& appId, http::PeerId peer,
             const common::InfoHash &infoHash, PeerServiceT &peerService,
             disk::DiskEventQueue::LeftEndPoint diskQueue, PieceStateManager &pieceRepository);

    bool sendHandShake(std::chrono::nanoseconds now);

    ProtocolState onMessage(const HandShake &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const KeepAlive &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const Choke &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const UnChoke &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const Interested &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const NotInterested &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const Have &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const Bitfield &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const Request &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const Piece &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const Cancel &hs, std::chrono::nanoseconds now);
    ProtocolState onMessage(const Port &hs, std::chrono::nanoseconds now);

    const common::InfoHash &getInfoHash() const;

  private:
    void sendInterested(std::chrono::nanoseconds now);
    ProtocolState requestNextPiece(std::chrono::nanoseconds now);
    PieceState *getNextAvailablePiece();

  private:
    bool mAmChoking{true};
    bool mAmInterested{false};
    bool mPeerChoking{true};
    bool mPeerInterested{false};

    common::AppId appId;
    http::PeerId peer;
    common::InfoHash infoHash;
    std::unordered_set<uint32_t> availablePieces;
    PeerServiceT &peerService;
    disk::DiskEventQueue::LeftEndPoint diskQueue;
    PieceStateManager &pieceRepository;
};
} // namespace fractals::network::p2p