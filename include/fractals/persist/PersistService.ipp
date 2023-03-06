#include "fractals/common/utils.h"
#include "fractals/persist/Event.h"
#include "fractals/persist/PersistService.h"

#include "fractals/common/CurlPoll.h"
#include <chrono>
#include <ctime>
#include <thread>

namespace fractals::persist
{
template <typename TrackerClientT>
PersistServiceImpl<TrackerClientT>::PersistServiceImpl(PersistEventQueue::RightEndPoint queue, TrackerClientT &client)
    : requestQueue(queue), client(client)
{
}

template <typename TrackerClientT> TrackerClientT &PersistServiceImpl<TrackerClientT>::getClient()
{
    return client;
}

template <typename TrackerClientT> void PersistServiceImpl<TrackerClientT>::disable()
{
    running = false;
}

template <typename TrackerClientT> void PersistServiceImpl<TrackerClientT>::pollForever()
{
    running = true;
    while (running)
    {
        if (!pollOnce())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

template <typename TrackerClientT> bool PersistServiceImpl<TrackerClientT>::pollOnce()
{
    if (requestQueue.canPop())
    {
        const auto req = requestQueue.pop();

        client.query(req, std::chrono::milliseconds(5000));

        std::visit(
            common::overloaded{
                [&](const AddTorrent &req) { client.addTorrent(req); },
                [&](const RemoveTorrent &req) { client.deleteTorrent(req.infoHash); },
                [&](const LoadTorrent &req) {
                    const auto torr = client.loadTorrent(req.infoHash);
                    if (torr)
                    {
                        requestQueue.push(Torrents{torr});
                    }
                },
                [&](const LoadTorrents &req) {
                    const auto torrs = client.loadTorrents();
                    requestQueue.push(Torrents{torrs});
                },
                [&](const AddPiece &req) { client.addPiece(req); },
                [&](const RemovePieces &req) { client.deletePiece(req.infoHash); },
                [&](const LoadPieces &req) { requestQueue.push(Pieces{client.loadPieces(req.infoHash)}); },
                [&](const AddAnnounce &req) { client.addAnnounce(req); },
                [&](const RemoveAnnounces &req) { client.deleteAnnounce(req.infoHash); },
                [&](const LoadAnnounces &req) { requestQueue.push(Announces{client.loadAnnounces(req.infoHash)}); },
            },
            req);
    }
}

} // namespace fractals::persist