#include "fractals/common/Tagged.h"
#include "fractals/network/http/Peer.h"
#include <fractals/network/p2p/PeerTracker.h>
#include <spdlog/spdlog.h>
#include <unordered_set>

namespace fractals::network::p2p
{

uint16_t PeersInfo::getKnownPeerCount() const
{
    return inactivePeers.size() + activePeers.size();
}

uint16_t PeersInfo::getActivePeerCount() const
{
    return activePeers.size();
}

uint16_t PeersInfo::getConnectedPeerCount() const
{
    return connected;
}

const std::unordered_set<http::PeerId> &PeersInfo::getActivePeers() const
{
    return activePeers;
}

void PeersInfo::addActivePeer(http::PeerId peer)
{
    activePeers.emplace(peer);
}

void PeersInfo::incrConnected()
{
    ++connected;
}

void PeersInfo::makeInactive(http::PeerId peer)
{
    inactivePeers.emplace(peer);
    activePeers.erase(peer);
}

void PeersInfo::makeAllInactive()
{
    for (http::PeerId peer : activePeers)
    {
        inactivePeers.emplace(peer);
    }

    activePeers.clear();
}

void PeerTracker::activateTorrent(const common::InfoHash &ih)
{
    activeTorrents.emplace(ih);
    peersInfoMap.emplace(ih, PeersInfo{});
}

std::vector<PeerCommand> PeerTracker::deactivateTorrent(const common::InfoHash &ih)
{
    activeTorrents.erase(ih);

    std::vector<PeerCommand> cmds;
    for (const auto peer : peersInfoMap[ih].getActivePeers())
    {
        cmds.emplace_back(PeerCommand{PeerCommandFlag::DISCONNECT, peer, ih});
    }

    return cmds;
}

std::vector<PeerCommand> PeerTracker::onPeerDisconnect(const http::PeerId &peer)
{
    auto it = peerMap.find(peer);
    if (it == peerMap.end())
    {
        spdlog::error("PeerTracker::onPeerDisconnect. Could not find peer.");
        return {};
    }

    peersInfoMap[it->second.torrent].makeInactive(peer);
    peerMap.erase(peer);
    currConnectedPeers--;

    spdlog::info(
        "PeerTracker::onPeerDisconnect. currConnectedPeers={} torrent={} connectedToTorr={}",
        currConnectedPeers, it->second.torrent,
        peersInfoMap[it->second.torrent].getConnectedPeerCount());

    if (activeTorrents.contains(it->second.torrent))
    {
        return makeCommand();
    }

    return {};
}

std::vector<PeerCommand> PeerTracker::onPeerConnect(const http::PeerId &peer)
{
    const auto &torr = peerMap[peer].torrent;
    peerMap[peer].connected = PeerConnection::CONNECTED;
    peerMap[peer].hasConnectedBefore = true;
    peersInfoMap[torr].incrConnected();

    spdlog::info("PeerTracker::onPeerConnect. currConnectedPeers={} torrent={} connectedToTorr={}",
                 currConnectedPeers, torr, peersInfoMap[torr].getConnectedPeerCount());

    if (activeTorrents.contains(torr))
    {
        return makeCommand();
    }

    return {};
}

std::vector<PeerCommand> PeerTracker::onAnnounce(const http::Announce &announce)
{
    for (const auto &peerId : announce.peers)
    {
        if (!peerMap.contains(peerId))
        {
            peerMap[peerId] = Peer{PeerConnection::NOT_CONNECTED, peerId, announce.infoHash, false};
        }
    }

    if (!peersInfoMap.contains(announce.infoHash))
    {
        peersInfoMap[announce.infoHash].makeAllInactive();
    }

    return makeCommand();
}

uint16_t PeerTracker::getKnownPeerCount(const common::InfoHash &ih) const
{
    return peersInfoMap.at(ih).getKnownPeerCount();
}

uint16_t PeerTracker::getConnectedPeerCount(const common::InfoHash &ih) const
{
    return peersInfoMap.at(ih).getConnectedPeerCount();
}

std::vector<PeerCommand> PeerTracker::makeCommandFor(const common::InfoHash &torrent)
{
    std::vector<PeerCommand> cmds;
    while (peersInfoMap[torrent].getActivePeerCount() < maxPeersPerTorrent &&
           currConnectedPeers < maxConnectedPeers)
    {
        auto peer = findNotConnectedPeer(torrent);

        if (peer)
        {
            peer->connected = PeerConnection::CONNECTING;
            currConnectedPeers++;
            peersInfoMap[torrent].addActivePeer(peer->peer);

            cmds.emplace_back(PeerCommand{PeerCommandFlag::TRY_CONNECT, peer->peer, torrent});
        }
        else
        {
            break;
        }
    }

    if (cmds.empty() && peersInfoMap[torrent].getActivePeerCount() < maxPeersPerTorrent)
    {
        return std::vector<PeerCommand>{requestAnnounce(torrent)};
    }
    else
    {
        return cmds;
    }
}

std::vector<PeerCommand> PeerTracker::makeCommand()
{
    std::vector<PeerCommand> cmds;
    for (auto &[torrent, _] : peersInfoMap)
    {
        const auto torrCmds = makeCommandFor(torrent);
        cmds.insert(cmds.end(), torrCmds.begin(), torrCmds.end());
    }

    return cmds;
}

PeerCommand PeerTracker::requestAnnounce(const common::InfoHash &torrent)
{
    if (activeTorrents.contains(torrent))
    {
        return PeerCommand{PeerCommandFlag::DO_ANNOUNCE, {}, torrent};
    }

    return {};
}

PeerTracker::Peer *PeerTracker::findNotConnectedPeer(const common::InfoHash &torrent)
{
    auto it = std::find_if(peerMap.begin(), peerMap.end(),
                           [](const auto &kvp)
                           {
                               return kvp.second.connected == PeerConnection::NOT_CONNECTED &&
                                      !kvp.second.hasConnectedBefore;
                           });

    if (it == peerMap.end())
    {
        return nullptr;
    }
    else
    {
        return &(it->second);
    }
}

PeerTracker::Peer *PeerTracker::findConnectingPeer(const common::InfoHash &torrent)
{
    auto it = std::find_if(peerMap.begin(), peerMap.end(),
                           [](const auto &kvp)
                           {
                               return kvp.second.connected == PeerConnection::CONNECTING;
                           });

    if (it == peerMap.end())
    {
        return nullptr;
    }
    else
    {
        return &(it->second);
    }
}
} // namespace fractals::network::p2p