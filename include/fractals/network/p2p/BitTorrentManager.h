#pragma once

#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
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

    void handleEvent(EpollServiceResponse &&event)
    {
        std::visit(common::overloaded{
                       [&](EpollError &&err) { caller->shutdown(std::to_string(err.errorCode)); },
                       [&](ReadEvent &&msg) { handleEvent(std::move(msg)); },
                       [&](ConnectionCloseEvent &&close) { caller->closeConnection(close.peerId); },
                       [&](ReadEventResponse &&err) { caller->dropPeer(err.peerId, std::to_string(err.errorCode)); },
                       [&](ConnectionError &&err) { caller->dropPeer(err.peerId, std::to_string(err.errorCode)); },
                       [&](Deactivate &&deact) { caller->onDeactivate(); },
                       [&](auto &&msg) {} },
                   std::move(event));
    }

    void handleEvent(ReadEvent &&event)
    {
        // clang-format off
        // std::visit(common::overloaded{
        //     [&](SerializeError &&error) {},
        //     [&](auto &&msg) 
        //     {
        //         using T = std::decay_t<decltype(msg)>;
        //         if constexpr (!std::is_same_v<T, SerializeError>)
        //         {
        //             const auto state = caller->forwardToPeer(event.peer, std::move(msg));
        //             switch (state)
        //             {
        //             case ProtocolState::ERROR:
        //                 caller->shutdown("Internal state error of pieces");
        //                 break;

        //             case ProtocolState::HASH_CHECK_FAIL:
        //                 caller->dropPeer(event.peer.getId(), "HashCheckFailure");
        //                 break;

        //             case ProtocolState::CLOSED:
        //                 caller->closeConnection(event.peer.getId());
        //                 break;

        //             case ProtocolState::OPEN:
        //                 break;
        //             }
        //         }
        //     }},
        // event.mMessage);
        // clang-format on
    }

  private:
    Caller *caller;
};

class BitTorrentManager
{
  public:
    enum class State
    {
        InActive,
        Active,
        Deactivating
    };

    BitTorrentManager(EpollMsgQueue::LeftEndPoint peerQueue, PeerService &peerService,
                      persist::PersistEventQueue::LeftEndPoint persistQueue, disk::DiskEventQueue &diskQueue);

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
    BitTorrentEventHandler<BitTorrentManager> eventHandler;

    // Peer responses available in queue
    EpollMsgQueue::LeftEndPoint peerQueue;
    // Writing to peer must go through BufferedQueueManager
    PeerService &peerService;

    // Internal state of pieces downloaded and missing
    std::unordered_map<std::string, PieceStateManager> pieceMan;
    std::unordered_map<http::PeerId, Protocol> connections;

    // Persist to db and disk
    persist::PersistEventQueue::LeftEndPoint persistQueue;
    disk::DiskEventQueue &diskQueue;
};

} // namespace fractals::network::p2p