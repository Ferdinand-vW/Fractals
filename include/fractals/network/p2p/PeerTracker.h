#pragma once

#include "fractals/common/Tagged.h"
#include "fractals/network/http/Announce.h"
#include "fractals/network/http/Peer.h"
#include <cstdint>

namespace fractals::network::p2p
{
enum class PeerCommandFlag
{
    TRY_CONNECT,
    DISCONNECT,
    DO_ANNOUNCE,
    NOTHING,
};

struct PeerCommand
{
    PeerCommandFlag command{PeerCommandFlag::NOTHING};
    http::PeerId peer;
    common::InfoHash torrent;
};

class PeerTracker
{
    enum class State
    {
        NotActive,
        Active
    };
    enum class PeerConnection
    {
        NOT_CONNECTED,
        CONNECTING,
        CONNECTED
    };

    struct Peer
    {
        PeerConnection connected{PeerConnection::NOT_CONNECTED};
        http::PeerId peer;
        common::InfoHash torrent;
        bool hasConnectedBefore{false};
    };

  public:
    void activateTorrent(const common::InfoHash& ih);
    std::vector<PeerCommand> deactivateTorrent(const common::InfoHash &ih);
    [[nodiscard]] std::vector<PeerCommand> onPeerDisconnect(const http::PeerId &peer);
    [[nodiscard]] std::vector<PeerCommand> onPeerConnect(const http::PeerId &peer);
    [[nodiscard]] std::vector<PeerCommand> onAnnounce(const http::Announce &announce);

  private:
    std::vector<PeerCommand> makeCommandFor(const common::InfoHash &torrent);
    std::vector<PeerCommand> makeCommand();

    Peer *findNotConnectedPeer(const common::InfoHash &torrent);
    Peer *findConnectingPeer(const common::InfoHash &torrent);
    PeerCommand requestAnnounce(const common::InfoHash &torrent);

    uint16_t maxConnectedPeers{200};
    uint16_t currConnectedPeers{0};
    uint16_t maxPeersPerTorrent{20};

    State state{State::NotActive};
    std::unordered_map<http::PeerId, Peer> peerMap;
    std::unordered_map<common::InfoHash, std::unordered_set<http::PeerId>> peerCountMap;
    std::unordered_set<common::InfoHash> activeTorrents;
};
} // namespace fractals::network::p2p
