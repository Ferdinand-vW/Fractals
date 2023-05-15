#pragma once

#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/PeerEvent.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/network/p2p/PieceStateManager.h"
#include "fractals/network/p2p/Protocol.h"
#include "fractals/persist/PersistEventQueue.h"
#include <condition_variable>
#include <type_traits>
namespace fractals::network::p2p
{

template <typename Caller> class BitTorrentEventHandler
{
  public:
    BitTorrentEventHandler(Caller *caller) : caller(caller)
    {
    }

    void handleEvent(PeerEvent &&event)
    {
        std::visit(common::overloaded{[&](Shutdown &&err) { caller->shutdown(); },
                                      [&](Disconnect &&msg) { caller->dropPeer(msg.peerId); },
                                      [&](Message &&msg) { handleMessage(std::move(msg)); }},
                   std::move(event));
    }

    void handleMessage(Message &&event)
    {
        // clang-format off
        std::visit(common::overloaded{
            [&](SerializeError &&error) {},
            [&](auto &&msg) 
            {
                using T = std::decay_t<decltype(msg)>;
                if constexpr (!std::is_same_v<T, SerializeError>)
                {
                    const auto state = caller->template forwardToPeer<T>(event.peer, std::move(msg));
                    switch (state)
                    {
                    case ProtocolState::ERROR:
                        caller->shutdown();
                        break;

                    case ProtocolState::HASH_CHECK_FAIL:
                        caller->dropPeer(event.peer);
                        break;

                    case ProtocolState::CLOSED:
                        caller->dropPeer(event.peer);
                        break;

                    case ProtocolState::OPEN:
                        break;
                    }
                }
            }},
        event.message);
        // clang-format on
    }

  private:
    Caller *caller;
};

template <typename PeerServiceT> class BitTorrentManagerImpl
{
  public:
    enum class State
    {
        InActive,
        Active,
        Deactivating
    };

    BitTorrentManagerImpl(PeerServiceT &peerService, persist::PersistEventQueue::LeftEndPoint persistQueue,
                          disk::DiskEventQueue &diskQueue);

    void run();
    void eval(bool &&workDone);

    void shutdown(const std::string &reason);
    void onShutdown(uint8_t token);

    void onDeactivate();

    template <typename Msg> ProtocolState forwardToPeer(http::PeerId peerId, Msg &&msg)
    {
        auto it = connections.find(peerId);
        if (it != connections.end())
        {
            return it->second.onMessage(std::move(msg));
        }

        return ProtocolState::ERROR;
    }

    void closeConnection(http::PeerId peer);
    void dropPeer(http::PeerId peer, const std::string &reason);

  private:
    State state{State::InActive};
    std::unordered_set<uint8_t> tokens;

    std::mutex mutex;
    std::condition_variable cv;
    BitTorrentEventHandler<BitTorrentManagerImpl<PeerServiceT>> eventHandler;

    // Writing to peer must go through BufferedQueueManager
    PeerServiceT &peerService;

    // Internal state of pieces downloaded and missing
    std::unordered_map<std::string, PieceStateManager> pieceMan;
    std::unordered_map<http::PeerId, Protocol<PeerServiceT>> connections;

    // Persist to db and disk
    persist::PersistEventQueue::LeftEndPoint persistQueue;
    disk::DiskEventQueue &diskQueue;
};

using BitTorrentManager = BitTorrentManagerImpl<PeerService>;

} // namespace fractals::network::p2p