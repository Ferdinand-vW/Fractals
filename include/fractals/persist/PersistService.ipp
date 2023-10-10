#include "fractals/common/utils.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/PersistEventQueue.h"
#include "fractals/persist/PersistService.h"

#include "fractals/common/CurlPoll.h"
#include "fractals/sync/QueueCoordinator.h"
#include <chrono>
#include <ctime>
#include <thread>

namespace fractals::persist
{
template <typename PersistClientT>
PersistServiceImpl<PersistClientT>::PersistServiceImpl(sync::QueueCoordinator &coordinator,
                                                       PersistEventQueue::RightEndPoint btQueue,
                                                       AppPersistQueue::RightEndPoint appQueue,
                                                       PersistClientT &client)
    : coordinator(coordinator), btQueue(btQueue), appQueue(appQueue), client(client)
{
    coordinator.addAsPublisherForPersistService<PersistEventQueue>(btQueue);
    coordinator.addAsPublisherForPersistService<AppPersistQueue>(appQueue);
    client.openConnection("torrents.db");
}

template <typename PersistClientT> PersistClientT &PersistServiceImpl<PersistClientT>::getClient()
{
    return client;
}

template <typename PersistClientT> void PersistServiceImpl<PersistClientT>::disable()
{
    isActive = false;
}

template <typename PersistClientT> void PersistServiceImpl<PersistClientT>::run()
{
    isActive = true;
    while (isActive)
    {
        coordinator.waitOnPersistServiceUpdate();

        while (btQueue.canPop())
        {
            processBtEvent();
        }

        while (appQueue.canPop())
        {
            processAppEvent();
        }
    }

    spdlog::info("PersistService::run. Shutdown");
}

template <typename PersistClientT> void PersistServiceImpl<PersistClientT>::processBtEvent()
{
    auto &&req = std::move(btQueue.pop());

    std::visit(
        common::overloaded{
            [&](const AddTorrent &req)
            {
                std::visit(common::overloaded{[&](const auto &resp)
                                              {
                                                  btQueue.push(resp);
                                              }},
                           client.addTorrent(req));
            },
            [&](const RemoveTorrent &req)
            {
                client.deleteTorrent(req.infoHash);
            },
            [&](const AddTrackers& req)
            {
                client.addTrackers(req);
            },
            [&](const LoadTrackers& req)
            {
                btQueue.push(Trackers{req.infoHash, client.loadTrackers(req.infoHash)});
            },
            [&](const PieceComplete &req)
            {
                client.addPiece(req);
            },
            [&](const RemovePieces &req)
            {
                client.deletePieces(req.infoHash);
            },
            [&](const LoadPieces &req)
            {
                btQueue.push(Pieces{req.infoHash, client.loadPieces(req.infoHash)});
            },
            [&](const AddAnnounce &req)
            {
                client.addAnnounce(req);
            },
            [&](const RemoveAnnounces &req)
            {
                client.deleteAnnounce(req.infoHash);
            },
            [&](const LoadAnnounces &req)
            {
                btQueue.push(Announces{req.infoHash, client.loadAnnounces(req.infoHash)});
            },
            [&](const Shutdown &)
            {
                isActive = false;
            }},
        req);
}

template <typename PersistClientT> void PersistServiceImpl<PersistClientT>::processAppEvent()
{
    auto &&req = std::move(appQueue.pop());
    std::visit(
        common::overloaded{
            [&](const RequestStats &req)
            {
                for (const auto &[torrId, ih] : req.requested)
                {
                    auto torrStats = client.loadTorrentStats(ih);
                    torrStats.torrId = torrId;
                    appQueue.push(torrStats);
                }
            },

            [&](const LoadTorrents &req)
            {
                const auto torrs = client.loadTorrents();
                appQueue.push(AllTorrents{torrs});
            },
        },
        req);
}

} // namespace fractals::persist