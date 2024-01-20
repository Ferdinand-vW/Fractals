#pragma once

#include <fractals/common/Tagged.h>
#include <fractals/network/http/Announce.h>
#include <fractals/network/http/Peer.h>
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

class PeersInfo
{
    public:
        PeersInfo() = default;
        uint16_t getKnownPeerCount() const;
        uint16_t getActivePeerCount() const;
        uint16_t getConnectedPeerCount() const;
        const std::unordered_set<http::PeerId>& getActivePeers() const;
        void addActivePeer(http::PeerId);
        void incrConnected();
        void makeInactive(http::PeerId);
        void makeAllInactive();

    private:
    std::unordered_set<http::PeerId> inactivePeers;
    std::unordered_set<http::PeerId> activePeers;
    uint16_t connected{0};
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

    uint16_t getKnownPeerCount(const common::InfoHash&) const;
    uint16_t getConnectedPeerCount(const common::InfoHash&) const;

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
    std::unordered_map<common::InfoHash, PeersInfo> peersInfoMap;
    std::unordered_set<common::InfoHash> activeTorrents;
};
} // namespace fractals::network::p2p
