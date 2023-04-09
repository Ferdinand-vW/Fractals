#pragma once

#include "EpollService.h"
#include "fractals/common/TcpService.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/PeerEventQueue.h"
#include "fractals/network/p2p/PeerFd.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <optional>

namespace fractals::network::p2p
{

template <template <ActionType> typename EpollService, typename BufferedQueueManager, typename TcpService>
class PeerServiceImpl
{
  public:
    PeerServiceImpl(typename EpollService<ActionType::READ>::Epoll &readEpoll,
                    typename EpollService<ActionType::WRITE>::Epoll &writeEpoll, BufferedQueueManager &buffMan,
                    TcpService &tcpService)
        : buffMan(buffMan), reader(readEpoll, buffMan), writer(writeEpoll, buffMan), tcpService(tcpService)
    {
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
        }
        else
        {
            peerFd = it->second;
        }

        buffMan.sendToPeer(*peerFd, std::move(msg));
    }

    PeerEventQueue &getReadEndpoint();

    void deactivate();
    void shutdown();

  private:
    EpollService<ActionType::READ> reader;
    EpollService<ActionType::WRITE> writer;

    BufferedQueueManager &buffMan;
    TcpService& tcpService;

    std::unordered_map<http::PeerId, PeerFd> peerFds;
};

using PeerService = PeerServiceImpl<EpollService, BufferedQueueManager, common::TcpService>;
} // namespace fractals::network::p2p