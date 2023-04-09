#include "fractals/network/p2p/BitTorrentManager.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/PeerEventQueue.h"
#include "fractals/persist/PersistEventQueue.h"
#include <mutex>
#include <type_traits>
#include <variant>

namespace fractals::network::p2p
{

BitTorrentManager::BitTorrentManager(PeerEventQueue &peerQueue, PeerService &peerService,
                                     persist::PersistEventQueue::LeftEndPoint persistQueue,
                                     disk::DiskEventQueue &diskQueue)
    : eventHandler{this}, peerQueue(peerQueue), peerService(peerService), persistQueue(persistQueue),
      diskQueue(diskQueue)
{
    peerQueue.attachNotifier(&cv);
    persistQueue.attachNotifier(&cv);
};

void BitTorrentManager::run()
{
    state = State::InActive;

    while (state != State::InActive)
    {
        bool workDone{false};
        eval(std::move(workDone));

        if (!workDone)
        {
            std::unique_lock lck(mutex);
            cv.wait(lck, [&]() { return !peerQueue.isEmpty() || persistQueue.canPop(); });
        }
    }
}

void BitTorrentManager::eval(bool &&workDone)
{
    if (!peerQueue.isEmpty())
    {
        auto &&event = peerQueue.pop();

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

void BitTorrentManager::shutdown(const std::string &reason)
{
    state = State::Deactivating;

    peerService.shutdown();
}

void BitTorrentManager::closeConnection(http::PeerId)
{
}

void BitTorrentManager::dropPeer(http::PeerId, const std::string &err)
{
}

} // namespace fractals::network::p2p