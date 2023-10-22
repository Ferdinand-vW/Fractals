#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <spdlog/spdlog.h>
namespace fractals::sync
{
class QueueCoordinator
{
  private:
    using Predicate = std::function<bool(void)>;

    template <typename Q> using Left = typename Q::LeftEndPoint;
    template <typename Q> using Right = typename Q::RightEndPoint;

  public:
    template <typename Queue> void addAsPublisherForBitTorrentManager(Left<Queue> left)
    {
        left.attachNotifier(btManMutex, btManCv);
        btManPreds.emplace_back(
            [left]()
            {
                return left.canPop();
            });
    }

    void waitOnBtManUpdate(std::chrono::milliseconds timeout)
    {
        waitOnNotify(btManMutex, btManCv, btManPreds, btManForceNotified, timeout);
    }

    void forceNotifyBtMan()
    {
        forceNotify(btManMutex, btManCv, btManForceNotified);
    }

    template <typename Queue> void addAsPublisherForAnnounceService(Right<Queue> right)
    {
        right.attachNotifier(annServiceMutex, annServiceCv);
        annServicePreds.emplace_back(
            [right]()
            {
                return right.canPop();
            });
    }

    void
    waitOnAnnounceServiceUpdate(std::chrono::milliseconds timeout = std::chrono::milliseconds(0))
    {
        waitOnNotify(annServiceMutex, annServiceCv, annServicePreds, annServiceForceNotified,
                     timeout);
    }

    void forceNotifyAnnounceService()
    {
        forceNotify(annServiceMutex, annServiceCv, annServiceForceNotified);
    }

    template <typename Queue> void addAsPublisherForPersistService(Right<Queue> right)
    {
        right.attachNotifier(persistServiceMutex, persistServiceCv);
        persistServicePreds.emplace_back(
            [right]()
            {
                return right.canPop();
            });
    }

    void waitOnPersistServiceUpdate()
    {
        waitOnNotify(persistServiceMutex, persistServiceCv, persistServicePreds,
                     persistServiceForceNotified);
    }

    void forceNotifyPersistService()
    {
        forceNotify(persistServiceMutex, persistServiceCv, persistServiceForceNotified);
    }

    template <typename Queue> void addAsPublisherForDiskService(Right<Queue> right)
    {
        right.attachNotifier(diskServiceMutex, diskServiceCv);
        diskServicePreds.emplace_back(
            [right]()
            {
                return right.canPop();
            });
    }

    void waitOnDiskServiceUpdate()
    {
        waitOnNotify(diskServiceMutex, diskServiceCv, diskServicePreds, diskServiceForceNotified);
    }

    void forceNotifyDiskService()
    {
        forceNotify(diskServiceMutex, diskServiceCv, diskServiceForceNotified);
    }

  private:
    void waitOnNotify(std::mutex &mutex, std::condition_variable &cv,
                      const std::vector<Predicate> &preds, bool &forceNotified,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(0))
    {
        std::unique_lock<std::mutex> _lock(mutex);
        if (timeout == std::chrono::milliseconds(0))
        {
            while (std::none_of(preds.begin(), preds.end(),
                                [](auto p)
                                {
                                    return p();
                                }) ||
                   forceNotified)
            {
                cv.wait(_lock);
            }
        }
        else
        {

            cv.wait_for(_lock, timeout,
                        [&]()
                        {
                            return std::any_of(preds.begin(), preds.end(),
                                                [](auto p)
                                                {
                                                    return p();
                                                }) ||
                                   forceNotified;
                        });
        }

        forceNotified = false;
    }

    void forceNotify(std::mutex &mutex, std::condition_variable &cv, bool &forceNotified)
    {
        std::unique_lock<std::mutex> _lock(mutex);
        forceNotified = true;
        cv.notify_all();
    }

    std::mutex btManMutex;
    std::condition_variable btManCv;
    std::vector<Predicate> btManPreds;
    bool btManForceNotified{false};

    std::mutex annServiceMutex;
    std::condition_variable annServiceCv;
    std::vector<Predicate> annServicePreds;
    bool annServiceForceNotified{false};

    std::mutex persistServiceMutex;
    std::condition_variable persistServiceCv;
    std::vector<Predicate> persistServicePreds;
    bool persistServiceForceNotified{false};

    std::mutex diskServiceMutex;
    std::condition_variable diskServiceCv;
    std::vector<Predicate> diskServicePreds;
    bool diskServiceForceNotified{false};
};
} // namespace fractals::sync