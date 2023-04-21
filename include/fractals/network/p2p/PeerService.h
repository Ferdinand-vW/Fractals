#pragma once

#include "EpollService.h"
#include "fractals/common/TcpService.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/PeerFd.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <optional>

namespace fractals::network::p2p
{

template <typename EpollService, typename BufferedQueueManager, typename TcpService>
class PeerServiceImpl
{
  public:
    PeerServiceImpl(EpollService& epollService,
                    BufferedQueueManager &buffMan, TcpService &tcpService)
        : buffMan(buffMan), epollService(epollService), tcpService(tcpService)
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

            epollService.getCommQueue().push(Subscribe{*peerFd});
        }
        else
        {
            peerFd = it->second;
        }

        buffMan.addToWriteBuffer(*peerFd, std::move(encoder.encode(msg)));

        epollService.notify();
    }

    void deactivate();
    void shutdown();

  private:
    EpollService epollService;

    BitTorrentEncoder encoder;
    BufferedQueueManager &buffMan;
    TcpService &tcpService;

    std::unordered_map<http::PeerId, PeerFd> peerFds;
};

using PeerService = PeerServiceImpl<EpollService, BufferedQueueManager, common::TcpService>;
} // namespace fractals::network::p2p