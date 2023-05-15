#include "fractals/network/p2p/BitTorrentManager.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/persist/PersistEventQueue.h"
#include <mutex>
#include <type_traits>
#include <variant>

namespace fractals::network::p2p
{

template <typename PeerServiceT>
BitTorrentManagerImpl<PeerServiceT>::BitTorrentManagerImpl(PeerServiceT &peerService,
                                     persist::PersistEventQueue::LeftEndPoint persistQueue,
                                     disk::DiskEventQueue &diskQueue)
    : eventHandler{this}, peerService(peerService), persistQueue(persistQueue),
      diskQueue(diskQueue)
{
    peerService.subscribe(mutex, cv);
    persistQueue.attachNotifier(&mutex, &cv);
};

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::run()
{
    state = State::InActive;

    while (state != State::InActive)
    {
        bool workDone{false};
        eval(std::move(workDone));

        if (!workDone)
        {
            std::unique_lock lck(mutex);
            cv.wait(lck, [&]() { return peerService.canRead() || persistQueue.canPop(); });
        }
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::eval(bool &&workDone)
{
    if (peerService.canRead())
    {
        auto &&event = peerService.read();

        eventHandler.handleEvent(std::move(event));
        // do something
        workDone = true;
    }

    if (persistQueue.canPop())
    {
        persistQueue.pop();
        // do something

        workDone = true;
    }
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::shutdown(const std::string &reason)
{
    state = State::Deactivating;

    peerService.shutdown();
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::closeConnection(http::PeerId)
{
}

template <typename PeerServiceT>
void BitTorrentManagerImpl<PeerServiceT>::dropPeer(http::PeerId, const std::string &err)
{
}

} // namespace fractals::network::p2p