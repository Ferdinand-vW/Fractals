#pragma once

#include "EpollService.h"
#include "fractals/common/TcpService.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollServiceEvent.h"
#include "fractals/network/p2p/PeerEvent.h"
#include "fractals/network/p2p/PeerFd.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <condition_variable>
#include <fcntl.h>
#include <netinet/in.h>
#include <optional>
#include <string>

namespace fractals::network::p2p
{

template <typename EpollService, typename TcpService> class PeerServiceImpl
{
  public:
    PeerServiceImpl(EpollMsgQueue::LeftEndPoint queue, EpollService &epollService, TcpService &tcpService)
        : queue(queue), epollService(epollService), tcpService(tcpService)
    {
    }

    void subscribe(std::mutex &mtx, std::condition_variable &cv)
    {
        queue.attachNotifier(&mtx, &cv);
    }

    void write(http::PeerId peer, BitTorrentMessage &&msg)
    {
        const auto it = peerFds.find(peer);
        std::optional<PeerFd> peerFd{std::nullopt};
        if (it == peerFds.end())
        {
            int32_t fd = tcpService.connect(peer.m_ip, peer.m_port);
            if (fd < 0)
            {
                return;
            }

            peerFd = PeerFd{peer, fd};
            peerFds.emplace(peer, *peerFd);

            queue.push(Subscribe{*peerFd});
        }
        else
        {
            peerFd = it->second;
        }

        queue.push(WriteEvent{*peerFd, std::move(encoder.encode(msg))});

        epollService.notify();
    }

    std::optional<PeerEvent> read()
    {
        return std::visit(
            common::overloaded{
                [&](EpollError &&event) {
                    spdlog::error("Received critical error from epoll service: {}", std::to_string(event.errorCode));
                    shutdown();
                    return Shutdown{};
                },
                [&](ReadEventResponse &&event) {
                    spdlog::error("Encountered peer read error: {}", std::to_string(event.errorCode));
                    disconnectClient(event.peerId.getId());
                    return Disconnect{event.peerId.getId()};
                },
                [&](ReadEvent &&event) {
                    return Message{event.peer.getId(), encoder.decode(event.mMessage)};
                },
                [](WriteEventResponse &&event) {
                    spdlog::error("Encountered peer write error: {}", event.errorMsg);
                    return Disconnect{event.peer.getId()};
                },
                [](CtlResponse &&event) {
                    if (!event.errorMsg.empty())
                    {
                        spdlog::error("Encountered peer(id={}) subscription response: {}",
                                      event.peer.getId().toString(), event.errorMsg);
                        return Disconnect{event.peer.getId()};
                    }
                    return std::nullopt;
                },
                [](ConnectionCloseEvent &&event) {
                    spdlog::info("Connection with peer(id={}) has been closed", event.peerId.toString());
                    return Disconnect{event.peerId};
                },
                [](ConnectionError &&event) {
                    spdlog::error("Encountered peer connection error: {}", std::to_string(event.errorCode));
                    return Disconnect{event.peerId};
                },
                [](DeactivateResponse &&event) {
                    spdlog::info("Deactivated epoll service");
                    return std::nullopt;
                }},
            std::move(queue.pop()));
    }

    void deactivate()
    {
    }
    void shutdown()
    {
    }
    void disconnectClient(http::PeerId)
    {
    }

    bool canRead()
    {
        return queue.numToRead();
    }

  private:
    EpollMsgQueue::LeftEndPoint queue;
    EpollService &epollService;

    BitTorrentEncoder encoder;
    TcpService &tcpService;

    std::unordered_map<http::PeerId, PeerFd> peerFds;
};

using PeerService = PeerServiceImpl<EpollService, common::TcpService>;
} // namespace fractals::network::p2p