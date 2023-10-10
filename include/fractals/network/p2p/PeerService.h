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
#include <chrono>
#include <condition_variable>
#include <fcntl.h>
#include <netinet/in.h>
#include <optional>
#include <ratio>
#include <string>
#include <variant>

namespace fractals::network::p2p
{

template <typename EpollService, typename TcpService> class PeerServiceImpl
{
  public:
    PeerServiceImpl(EpollMsgQueue::LeftEndPoint queue, EpollService &epollService,
                    TcpService &tcpService)
        : queue(queue), epollService(epollService), tcpService(tcpService)
    {
    }

    EpollMsgQueue::LeftEndPoint getQueueEndPoint()
    {
        return queue;
    }

    bool connect(http::PeerId peer, std::chrono::nanoseconds time)
    {
        if (!epollService.isActive())
        {
            spdlog::error("PeerService::connect. EpollService not ready");
            return false;
        }

        peerFds.erase(peer);
        handShaked.erase(peer);

        int32_t fd = tcpService.connect(peer.m_ip, peer.m_port);

        if (fd < 0)
        {
            return false;
        }

        spdlog::info("PeerService::connect to peer={}", peer.toString());
        const PeerFd peerFd{peer, fd};
        peerFds.emplace(peer, peerFd);
        timestamps.emplace(peer, time);
        queue.push(Subscribe{peerFd});
        epollService.notify();

        return true;
    }

    bool write(http::PeerId peer, BitTorrentMessage &&msg, std::chrono::nanoseconds time)
    {
        if (!epollService.isActive())
        {
            spdlog::error("PeerService::write. EpollService not ready");
            return false;
        }

        const auto it = peerFds.find(peer);
        if (it == peerFds.end())
        {
            spdlog::error("PeerService::write. Peer {} has no active connection", peer.toString());
            return false;
        }

        queue.push(WriteEvent{it->second, std::move(encoder.encode(msg))});

        epollService.notify();

        timestamps[peer] = time;

        return true;
    }

    std::optional<PeerEvent> read(std::chrono::nanoseconds time)
    {
        if (!epollService.isActive())
        {
            return std::nullopt;
        }

        return std::visit(
            common::overloaded{
                [&](EpollError &&event) -> std::optional<PeerEvent>
                {
                    spdlog::error(
                        "PeerService::EpollError. Received critical error from epoll service: {}",
                        std::to_string(event.errorCode));
                    shutdown();
                    return Shutdown{};
                },
                [&](ReadEventResponse &&event) -> std::optional<PeerEvent>
                {
                    disconnectClient(event.peerId.getId());
                    return ConnectionDisconnected{event.peerId.getId()};
                },
                [&](ReadEvent &&event) -> std::optional<PeerEvent>
                {
                    auto checkParseError = [&](const auto &decoded) -> std::optional<PeerEvent>
                    {
                        if (std::holds_alternative<SerializeError>(decoded))
                        {
                            static constexpr size_t MAX_SIZE{150};
                            int cappedSize = std::min(MAX_SIZE, event.mMessage.size());
                            common::string_view vw(event.mMessage.begin(),
                                                   event.mMessage.begin() + cappedSize);

                            const auto serError = std::get<SerializeError>(decoded);
                            spdlog::error("PeerService::ReadEvent. Serialization error. Peer={} "
                                          "error={} payload={}",
                                          event.peer.getId().toString(), serError.getError(),
                                          common::bytes_to_hex(vw));
                            return ConnectionDisconnected{event.peer.getId()};
                        }
                        else if (!std::holds_alternative<KeepAlive>(decoded))
                        {
                            timestamps[event.peer.getId()] = time;
                        }

                        return Message{event.peer.getId(), decoded};
                    };

                    if (handShaked.contains(event.peer.getId()))
                    {
                        return checkParseError(encoder.decode(event.mMessage));
                    }
                    else
                    {
                        handShaked.emplace(event.peer.getId());
                        return checkParseError(encoder.decodeHandShake(event.mMessage));
                    }
                },
                [&](ConnectionAccepted &&event) -> std::optional<PeerEvent>
                {
                    spdlog::debug("PeerService::ConnectionAccepted. Success");
                    timestamps[event.peer.getId()] = time;
                    return ConnectionEstablished{event.peer.getId()};
                },
                [&](WriteEventResponse &&event) -> std::optional<PeerEvent>
                {
                    if (event.errorMsg.empty())
                    {
                        timestamps[event.peer.getId()] = time;
                        return std::nullopt;
                    }
                    else
                    {
                        spdlog::error("PeerService::WriteEventResponxse. Peer write error: {}",
                                      event.errorMsg);
                        disconnectClient(event.peer.getId());
                        return ConnectionDisconnected{event.peer.getId()};
                    }
                },
                [&](CtlResponse &&event) -> std::optional<PeerEvent>
                {
                    if (!event.errorMsg.empty())
                    {
                        spdlog::error("Encountered peer(id={}) subscription response: {}",
                                      event.peer.getId().toString(), event.errorMsg);
                        disconnectClient(event.peer.getId());
                        return ConnectionDisconnected{event.peer.getId()};
                    }

                    return std::nullopt;
                },
                [&](ConnectionCloseEvent &&event) -> std::optional<PeerEvent>
                {
                    spdlog::debug(
                        "PeerService::ConnectionClose. Connection with peer(id={}) has been closed",
                        event.peerId.toString());
                    onClientDisconnect(event.peerId);
                    return ConnectionDisconnected{event.peerId};
                },
                [&](ConnectionError &&event) -> std::optional<PeerEvent>
                {
                    spdlog::error(
                        "PeerService::ConnectionError. Encountered peer {} connection error: {}",
                        event.peerId.toString(), std::to_string(event.errorCode));

                    onClientDisconnect(event.peerId);
                    return ConnectionDisconnected{event.peerId};
                },
            } // namespace fractals::network::p2p
            ,
            std::move(queue.pop()));
    }

    void shutdown()
    {
        queue.push(Deactivate{});
        epollService.notify();
    }

    // Epoll tells us client is disconnected
    void onClientDisconnect(http::PeerId peer)
    {
        if (peerFds.count(peer))
        {
            peerFds.erase(peer);
            handShaked.erase(peer);
            timestamps.erase(peer);
        }
    }

    // We tell epoll to disconnect client
    void disconnectClient(http::PeerId peer)
    {
        if (peerFds.count(peer))
        {
            queue.push(UnSubscribe{peerFds[peer]});
            epollService.notify();
        }
    }

    bool canRead()
    {
        return queue.numToRead() && epollService.isActive();
    }

    [[nodiscard]] std::vector<p2p::ConnectionDisconnected>
    activityCheck(std::chrono::nanoseconds now)
    {
        std::vector<p2p::ConnectionDisconnected> discs;
        auto it = timestamps.begin();
        while (it != timestamps.end())
        {
            const auto current = it;
            ++it;
            if (current->second + std::chrono::seconds(10) < now)
            {
                spdlog::info("PeerService::activityCheck. No activity with peer {}",
                             current->first.toString());
                disconnectClient(current->first);
                timestamps.erase(current);
            }
        }

        return discs;
    }

  private:
    EpollMsgQueue::LeftEndPoint queue;
    EpollService &epollService;

    BitTorrentEncoder encoder;
    TcpService &tcpService;

    std::unordered_map<http::PeerId, PeerFd> peerFds;
    std::unordered_map<http::PeerId, std::chrono::nanoseconds> timestamps;
    std::unordered_set<http::PeerId> handShaked;
};

using PeerService = PeerServiceImpl<EpollService, common::TcpService>;
} // namespace fractals::network::p2p