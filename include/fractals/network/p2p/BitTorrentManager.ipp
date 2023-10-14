#include "fractals/app/AppEventQueue.h"
#include "fractals/app/Client.h"
#include "fractals/app/Event.h"
#include "fractals/common/Tagged.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Announce.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/network/http/Event.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/http/Request.h"
#include "fractals/network/http/TrackerClient.h"
#include "fractals/network/p2p/BitTorrentManager.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/PeerEvent.h"
#include "fractals/network/p2p/PeerTracker.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/PersistEventQueue.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <mutex>
#include <type_traits>
#include <variant>

namespace fractals::network::p2p
{

template <typename PeerServiceT>
BitTorrentManagerImpl<PeerServiceT>::BitTorrentManagerImpl(
    sync::QueueCoordinator &coordinator, PeerServiceT &peerService,
    persist::PersistEventQueue::LeftEndPoint persistQueue,
    disk::DiskEventQueue::LeftEndPoint diskQueue,
    http::AnnounceEventQueue::LeftEndPoint announceQueue, app::AppEventQueue::LeftEndPoint appQueue)
    : clientId(app::generate_peerId()), peerEventHandler{this}, diskEventHandler{this},
      announceEventHandler{this}, persistEventHandler{this}, appEventHandler(this),
      peerService(peerService), coordinator(coordinator), persistQueue(persistQueue),
      diskQueue(diskQueue), announceQueue(announceQueue), appQueue(appQueue)
{
    coordinator.addAsPublisherForBitTorrentManager<EpollMsgQueue>(peerService.getQueueEndPoint());
    coordinator.addAsPublisherForBitTorrentManager<persist::PersistEventQueue>(persistQueue);
    coordinator.addAsPublisherForBitTorrentManager<disk::DiskEventQueue>(diskQueue);
    coordinator.addAsPublisherForBitTorrentManager<http::AnnounceEventQueue>(announceQueue);
    coordinator.addAsPublisherForBitTorrentManager<app::AppEventQueue>(appQueue);
};

template <typename PeerServiceT> void BitTorrentManagerImpl<PeerServiceT>::run()
{
    state = State::Active;

    while (state != State::InActive)
    {
        coordinator.waitOnBtManUpdate(std::chrono::milliseconds{0});
        currTime = std::chrono::system_clock::now().time_since_epoch();

        eval();

        for (const auto &event : peerService.activityCheck(currTime))
        {
            process(event);
        }
    }

    spdlog::info("BtMan::run. Shutdown");
}

template <typename PeerServiceT> void BitTorrentManagerImpl<PeerServiceT>::eval()
{
    if (peerService.canRead())
    {
        auto event = peerService.read(currTime);

        if (event)
        {
            peerEventHandler.handleEvent(event.value());
        }
        // do something
    }

    if (persistQueue.canPop())
    {
        auto &&event = persistQueue.pop();

        persistEventHandler.handleEvent(event);
    }

    if (announceQueue.canPop())
    {
        auto &&event = announceQueue.pop();
        announceEventHandler.handleEvent(event);
    }

    if (diskQueue.canPop())
    {
        auto &&event = diskQueue.pop();
        diskEventHandler.handleEvent(event);
    }

    if (appQueue.canPop())
    {
        auto &&event = appQueue.pop();
        appEventHandler.handleEvent(event);
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const app::AddTorrent &req)
{
    spdlog::info("BtMan::process(AddTorrent) File={}", req.filepath);
    diskQueue.push(disk::Read{req.filepath});
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const app::RemoveTorrent &req)
{
    spdlog::info("BtMan::process(RemoveTorrent) infoHash={}", req.infoHash);
    handlePeerCommands(peerTracker.deactivateTorrent(req.infoHash));
    pieceMan.erase(req.infoHash);
    torrents.erase(req.infoHash);

    auto it = connections.begin();
    while (it != connections.end())
    {
        if (it->second.getInfoHash() == req.infoHash)
        {
            it = connections.erase(it);
        }
        else
        {
            ++it;
        }
    }

    persistQueue.push(persist::RemoveTorrent{req.infoHash});
    announceQueue.push(http::DeleteTrackers{req.infoHash});
    appQueue.push(app::RemovedTorrent{req.infoHash});
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const app::StopTorrent &req)
{
    spdlog::info("BtMan::process(StopTorrent) infoHash={}", req.infoHash);
    handlePeerCommands(peerTracker.deactivateTorrent(req.infoHash));
    pieceMan[req.infoHash].setActive(false);
    appQueue.push(app::StoppedTorrent{req.infoHash});
    announceQueue.push(http::Pause{req.infoHash});
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const app::Shutdown &req)
{
    spdlog::info("BtMan::process(Shutdown)");
    announceQueue.push(http::Shutdown{});
    persistQueue.push(persist::Shutdown{});
    diskQueue.push(disk::Shutdown{});
    peerService.shutdown();

    appQueue.push(app::ShutdownConfirmation{});

    state = State::InActive;
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const app::RequestStats &req)
{
    spdlog::info("BtMan::process(RequestStats)");
    for (const auto &[torrId, infoHash] : req.requested)
    {
        appQueue.push(app::PeerStats{torrId, peerTracker.getKnownPeerCount(infoHash),
                                     peerTracker.getConnectedPeerCount(infoHash)});
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const persist::AddedTorrent &resp)
{
    appQueue.push(
        app::AddedTorrent{common::InfoHash{resp.torrent.infoHash}, resp.torrent, resp.files});
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const persist::TorrentExists &resp)
{
    appQueue.push(app::AddTorrentError{"Torrent already exists"});
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const persist::Announces &resp)
{
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const disk::ReadSuccess &resp)
{
    spdlog::info("BtMan::process(ReadSuccess) infoHash={}", resp.tm.getInfoHash());
    persistQueue.push(persist::AddTorrent(resp.tm, resp.readPath, resp.writePath));
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const app::StartTorrent &req)
{
    spdlog::info("BtMan::process(StartTorrent) infoHash={}", req.infoHash);
    auto torrIt = torrents.find(req.infoHash);
    if (torrIt == torrents.end())
    {
        torrIt = torrents.emplace(req.infoHash, TorrentState{req.torrent, false}).first;
    }

    if (torrIt->second.isComplete)
    {
        appQueue.push(app::CompletedTorrent{req.infoHash});
    }
    else
    {
        peerTracker.activateTorrent(req.infoHash);
        auto pieceIt = pieceMan.find(req.infoHash);
        if (pieceIt != pieceMan.end())
        {
            pieceIt->second.setActive(true);
        }
        else
        {
            PieceStateManager psm;
            psm.setActive(true);
            pieceMan.emplace(req.infoHash, psm);
        }

        appQueue.push(app::ResumedTorrent{req.infoHash});
        diskQueue.push(disk::InitTorrent{req.torrent, req.files});
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const app::ResumeTorrent &req)
{
    spdlog::info("BtMan::process(ResumeTorrent) infoHash={}", req.infoHash);
    auto torrIt = torrents.find(req.infoHash);
    if (torrIt == torrents.end())
    {
        spdlog::error("BtMan::process(ResumeTorrent) infoHash={}. Unknown torrent", req.infoHash);
    }

    if (torrIt->second.isComplete)
    {
        appQueue.push(app::CompletedTorrent{req.infoHash});
    }
    else
    {
        peerTracker.activateTorrent(req.infoHash);
        auto pieceIt = pieceMan.find(req.infoHash);
        if (pieceIt != pieceMan.end())
        {
            pieceIt->second.setActive(true);
        }
        else
        {
            spdlog::error("BtMan::process(ResumeTorrent) infoHash={}. Missing PieceStateManager",
                          req.infoHash);
        }

        appQueue.push(app::ResumedTorrent{req.infoHash});
        persistQueue.push(persist::LoadPieces{req.infoHash});
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const disk::TorrentInitialized &resp)
{
    spdlog::info("BtMan::process(TorrentInitialized). infoHash={}", resp.infoHash);
    persistQueue.push(persist::LoadPieces{resp.infoHash});
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const persist::Pieces &resp)
{
    spdlog::info("BtMan::process(Pieces). NumPieces={}", resp.result.size());

    auto &psm = pieceMan[resp.infoHash];
    psm.populate(resp.result);

    if (!psm.isAllComplete())
    {
        persistQueue.push(persist::LoadTrackers{resp.infoHash});
    }
    else
    {
        updateTorrentCompleted(resp.infoHash);
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const persist::Trackers &resp)
{
    spdlog::info("BtMan::process(Trackers) infoHash={}", resp.infoHash);

    auto it = torrents.find(resp.infoHash);
    if (it != torrents.end())
    {
        announceQueue.push(http::AddTrackers{resp.infoHash, resp.trackers});
        announceQueue.push(http::RequestAnnounce{resp.infoHash, it->second.model});
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const disk::ReadError &resp)
{
    appQueue.push(app::AddTorrentError{"Unable to read file: " + resp.error});
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const disk::WriteSuccess &resp)
{
    persistQueue.push(persist::PieceComplete{resp.infoHash, resp.pieceIndex});
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const disk::WriteError &resp)
{
    spdlog::error("BtMan::process(WriteError). Not Implemented");
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const http::Announce &resp)
{
    spdlog::info("BtMan::process(Announce)::Begin");
    handlePeerCommands(peerTracker.onAnnounce(resp));
    spdlog::info("BtMan::process(Announce)::End");
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const p2p::ConnectionDisconnected &resp)
{
    spdlog::info("BtMan::process(ConnectionDisconnected). peer={}", resp.peerId.toString());

    connections.erase(resp.peerId);
    handlePeerCommands(peerTracker.onPeerDisconnect(resp.peerId));
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::process(const p2p::ConnectionEstablished &resp)
{
    spdlog::info("BtMan::process(ConnectionEstablished). peer={}", resp.peer.toString());

    auto it = connections.find(resp.peer);

    if (it != connections.end())
    {
        handlePeerCommands(peerTracker.onPeerConnect(resp.peer));
        it->second.sendHandShake(currTime);
    }
    else
    {
        spdlog::error(
            "BtMan::process(ConnectionEstablished). Could not find connection for peer {}",
            resp.peer.toString());
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::disconnectPeer(const http::PeerId &peer)
{
    peerService.disconnectClient(peer);
}

template <typename PeerServiceT> void BitTorrentManagerImpl<PeerServiceT>::shutdown()
{
    state = State::Deactivating;

    peerService.shutdown();
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::peerCompletedTorrent(const http::PeerId &peer,
                                                               const common::InfoHash &ih)
{
    updateTorrentCompleted(ih);
    disconnectPeer(peer);
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::updateTorrentCompleted(const common::InfoHash &ih)
{
    auto &torrent = torrents[ih];
    if (!torrent.isComplete)
    {
        torrent.isComplete = true;
        handlePeerCommands(peerTracker.deactivateTorrent(ih));
        appQueue.push(app::CompletedTorrent{ih});
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::handlePeerCommands(const std::vector<PeerCommand> &cmds)
{
    for (const auto &cmd : cmds)
    {
        switch (cmd.command)
        {
        case PeerCommandFlag::TRY_CONNECT:
            spdlog::info("BtMan::handlePeerCommands. Attempt to connect to peer {}",
                         cmd.peer.toString());
            if (peerService.connect(cmd.peer, currTime))
            {
                auto &pm = *pieceMan.find(cmd.torrent);
                const auto [connIt, _] = connections.emplace(
                    cmd.peer, Protocol{clientId, cmd.peer, cmd.torrent, peerService, persistQueue,
                                       diskQueue, pm.second});
            }
            break;
        case PeerCommandFlag::DO_ANNOUNCE:
            spdlog::info("BtMan::handlePeerCommands. Request new announce for", cmd.torrent);
            assert(torrents.contains(cmd.torrent));
            announceQueue.push(http::RequestAnnounce{cmd.torrent, torrents[cmd.torrent].model});
            break;
        case PeerCommandFlag::DISCONNECT:
            spdlog::info("BtMan::handlePeerCommands. Disconnect peer {}", cmd.peer.toString());
            peerService.disconnectClient(cmd.peer);
            break;
        case PeerCommandFlag::NOTHING:
            break;
        }
    }
}

} // namespace fractals::network::p2p