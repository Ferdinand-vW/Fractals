
#include "fractals/AppId.h"
#include "fractals/app/AppEventQueue.h"
#include "fractals/app/TorrentController.h"
#include "fractals/common/TcpService.h"
#include "fractals/common/logger.h"
#include "fractals/common/stacktrace.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/disk/DiskIOService.h"
#include "fractals/network/http/AnnounceEventQueue.h"
#include "fractals/network/http/AnnounceService.h"
#include "fractals/network/http/TrackerClient.h"
#include "fractals/network/p2p/BitTorrentManager.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/EpollMsgQueue.h"
#include "fractals/network/p2p/EpollService.h"
#include "fractals/network/p2p/PeerFd.h"
#include "fractals/network/p2p/PeerService.h"
#include "fractals/persist/PersistClient.h"
#include "fractals/persist/PersistEventQueue.h"
#include "fractals/persist/PersistService.h"
#include "fractals/sync/QueueCoordinator.h"

#include <csignal>
#include <epoll_wrapper/Epoll.h>
#include <exception>
#include <iostream>
#include <thread>

using namespace fractals;

int main()
{
    // set up exceptions handlers
    std::set_terminate(&common::exceptionHandler);
    signal(SIGSEGV, common::segfaultHandler);

    common::setupLogging();

    Fractals::initAppId();

    sync::QueueCoordinator coordinator;

    network::p2p::EpollMsgQueue epollQueue;
    auto epoll = epoll_wrapper::Epoll<network::p2p::PeerFd>::epollCreate();
    if (!epoll)
    {
        throw "Could not create epoll instance";
    }
    network::p2p::BufferedQueueManager bufMan;
    network::p2p::EpollService epollSrvc{epoll.getEpoll(), bufMan, epollQueue.getRightEnd()};
    TcpService tcpSrvc;
    network::p2p::PeerService peerSrvc{epollQueue.getLeftEnd(), epollSrvc, tcpSrvc};

    persist::PersistEventQueue btPersistQueue;
    persist::AppPersistQueue appPersistQueue;
    persist::PersistClient persistClient;
    persist::PersistService persistSrvc{coordinator, btPersistQueue.getRightEnd(),
                                        appPersistQueue.getRightEnd(), persistClient};

    disk::DiskEventQueue diskQueue;
    disk::DiskIOService diskSrvc{coordinator, diskQueue.getRightEnd()};

    network::http::AnnounceEventQueue annQueue;
    network::http::TrackerClient trackerClient;
    network::http::AnnounceService annSrvc{coordinator, annQueue.getRightEnd(), trackerClient};

    app::AppEventQueue appQueue;

    network::p2p::BitTorrentManager btMan{coordinator,
                                          peerSrvc,
                                          btPersistQueue.getLeftEnd(),
                                          diskQueue.getLeftEnd(),
                                          annQueue.getLeftEnd(),
                                          appQueue.getLeftEnd()};

    spdlog::info("Starting threads..");

    auto rQueue = appQueue.getRightEnd();

    app::TorrentController controller(rQueue, appPersistQueue.getLeftEnd());

    std::thread thrdBtMan(
        [&btMan]()
        {
            btMan.run();
        });
    std::thread thrdAnnSrvc(
        [&annSrvc]()
        {
            annSrvc.run();
        });
    std::thread thrdDiskSrvc(
        [&diskSrvc]()
        {
            diskSrvc.run();
        });
    std::thread thrdPersistSrvc(
        [&persistSrvc]()
        {
            persistSrvc.run();
        });
    std::thread thrdEpollSrvc(
        [&epollSrvc]()
        {
            epollSrvc.run();
        });

    controller.run();

    // rQueue.push(app::AddTorrent{"torrents/1659464.torrent"});

    thrdAnnSrvc.join();
    thrdDiskSrvc.join();
    thrdPersistSrvc.join();
    thrdEpollSrvc.join();
    thrdBtMan.join();

    return 0;
}
