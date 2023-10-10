#include "fractals/common/Tagged.h"
#include "fractals/network/http/Peer.h"
#include <fractals/network/p2p/PeerTracker.h>
#include <spdlog/spdlog.h>

namespace fractals::network::p2p
{

void PeerTracker::activateTorrent(const common::InfoHash &ih)
{
    activeTorrents.emplace(ih);
}

std::vector<PeerCommand> PeerTracker::deactivateTorrent(const common::InfoHash &ih)
{
    activeTorrents.erase(ih);

    std::vector<PeerCommand> cmds;
    for (const auto peer : peerCountMap[ih])
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

    peerCountMap[it->second.torrent].erase(peer);
    peerMap.erase(peer);
    currConnectedPeers--;

    spdlog::info(
        "PeerTracker::onPeerDisconnect. currConnectedPeers={} torrent={} connectedToTorr={}",
        currConnectedPeers, it->second.torrent, peerCountMap[it->second.torrent].size());

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

    spdlog::info("PeerTracker::onPeerConnect. currConnectedPeers={} torrent={} connectedToTorr={}",
                 currConnectedPeers, torr, peerCountMap[torr].size());

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

    if (!peerCountMap.contains(announce.infoHash))
    {
        peerCountMap[announce.infoHash].clear();
    }

    return makeCommand();
}

std::vector<PeerCommand> PeerTracker::makeCommandFor(const common::InfoHash &torrent)
{
    std::vector<PeerCommand> cmds;
    while (peerCountMap[torrent].size() < maxPeersPerTorrent &&
           currConnectedPeers < maxConnectedPeers)
    {
        auto peer = findNotConnectedPeer(torrent);

        if (peer)
        {
            peer->connected = PeerConnection::CONNECTING;
            currConnectedPeers++;
            peerCountMap[torrent].emplace(peer->peer);

            cmds.emplace_back(PeerCommand{PeerCommandFlag::TRY_CONNECT, peer->peer, torrent});
        }
        else
        {
            break;
        }
    }

    if (cmds.empty() && peerCountMap[torrent].size() < maxPeersPerTorrent)
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
    for (auto &[torrent, _] : peerCountMap)
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