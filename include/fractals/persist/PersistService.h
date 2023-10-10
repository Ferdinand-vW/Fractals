#pragma once

#include "fractals/persist/Event.h"
#include "fractals/persist/PersistClient.h"
#include "fractals/persist/PersistEventQueue.h"
#include "fractals/sync/QueueCoordinator.h"

#include <unordered_map>

namespace fractals::persist
{

template <typename PersistClientT>
class PersistServiceImpl
{
  public:
    PersistServiceImpl(sync::QueueCoordinator& coordinator
    , PersistEventQueue::RightEndPoint btQueue
    , AppPersistQueue::RightEndPoint appQueue
    , PersistClientT &client);

    PersistClientT& getClient();

    void run();
    void processBtEvent();
    void processAppEvent();
    void disable();

  private:
    bool isActive{false};

    PersistEventQueue::RightEndPoint btQueue;
    AppPersistQueue::RightEndPoint appQueue;
    sync::QueueCoordinator& coordinator;
    PersistClientT& client;
};

using PersistService = PersistServiceImpl<PersistClient>;

} // namespace fractals::network::http