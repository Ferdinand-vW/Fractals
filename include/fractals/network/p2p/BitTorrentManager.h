#pragma once

#include "fractals/app/AppEventQueue.h"
#include "fractals/app/Client.h"
#include "fractals/app/Event.h"
#include "fractals/common/Tagged.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Announce.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/EventHandlers.h"
#include "fractals/network/p2p/PeerEvent.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/network/p2p/PeerTracker.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/network/p2p/Protocol.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/Models.h"
#include "fractals/persist/PersistEventQueue.h"
#include "fractals/sync/QueueCoordinator.h"
#include "fractals/torrent/TorrentMeta.h"

#include <chrono>
#include <condition_variable>
#include <type_traits>

namespace fractals::network::p2p
{

template <typename PeerServiceT> class BitTorrentManagerImpl
{
  public:
    using ThisType = BitTorrentManagerImpl<PeerServiceT>;

    struct TorrentState
    {
        persist::TorrentModel model;
        bool isComplete{false};
    };

    enum class State
    {
        InActive,
        Active,
        Deactivating
    };

    BitTorrentManagerImpl(sync::QueueCoordinator &coordinator, PeerServiceT &peerService,
                          persist::PersistEventQueue::LeftEndPoint persistQueue,
                          disk::DiskEventQueue::LeftEndPoint diskQueue,
                          http::AnnounceEventQueue::LeftEndPoint announceQueue,
                          app::AppEventQueue::LeftEndPoint appQueue);

    void run();
    void eval();

    void shutdown();
    void onShutdown(uint8_t token);

    void onDeactivate();

    template <typename Msg>
    std::pair<ProtocolState, common::InfoHash> forwardToPeer(http::PeerId peerId, const Msg &msg)
    {
        auto it = connections.find(peerId);
        if (it != connections.end())
        {
            return {it->second.onMessage(msg, currTime), it->second.getInfoHash()};
        }

        return {ProtocolState::CLOSED, {}};
    }

  public:
    void process(const app::AddTorrent &);
    void process(const app::RemoveTorrent &req);
    void process(const app::StopTorrent &req);
    void process(const app::StartTorrent &req);
    void process(const app::ResumeTorrent &req);
    void process(const app::Shutdown &req);
    void process(const app::RequestStats& req);
    void process(const persist::AddedTorrent &resp);
    void process(const persist::TorrentExists &resp);
    void process(const persist::Pieces &);
    void process(const persist::Trackers &);
    void process(const persist::Announces &);
    void process(const disk::ReadSuccess &);
    void process(const disk::ReadError &);
    void process(const disk::WriteSuccess &);
    void process(const disk::WriteError &);
    void process(const disk::TorrentInitialized &);
    void process(const http::Announce &);
    void process(const p2p::ConnectionDisconnected &);
    void process(const p2p::ConnectionEstablished &);

    void disconnectPeer(const http::PeerId &peer);
    void peerCompletedTorrent(const http::PeerId &peer, const common::InfoHash &ih);
    void updateTorrentCompleted(const common::InfoHash &ih);

  private:
    void handlePeerCommands(const std::vector<PeerCommand> &cmds);
    bool isComplete(const common::InfoHash &ih) const;

    common::AppId appId{};
    State state{State::InActive};
    std::unordered_set<uint8_t> tokens;
    std::chrono::nanoseconds currTime;

    PeerEventHandler<ThisType> peerEventHandler;
    DiskEventHandler<ThisType> diskEventHandler;
    AnnounceEventHandler<ThisType> announceEventHandler;
    PersistEventHandler<ThisType> persistEventHandler;
    AppEventHandler<ThisType> appEventHandler;

    PeerTracker peerTracker;
    // Writing to peer must go through BufferedQueueManager
    PeerServiceT &peerService;

    // Internal state of pieces downloaded and missing
    std::unordered_map<common::InfoHash, PieceStateManager> pieceMan;
    std::unordered_map<common::InfoHash, TorrentState> torrents;
    std::unordered_map<http::PeerId, Protocol<PeerServiceT>> connections;

    // Queues
    sync::QueueCoordinator &coordinator;
    persist::PersistEventQueue::LeftEndPoint persistQueue;
    disk::DiskEventQueue::LeftEndPoint diskQueue;
    http::AnnounceEventQueue::LeftEndPoint announceQueue;
    app::AppEventQueue::LeftEndPoint appQueue;
};

using BitTorrentManager = BitTorrentManagerImpl<PeerService>;

} // namespace fractals::network::p2p