#pragma once

#include "fractals/persist/PersistClient.h"
#include "fractals/persist/PersistEventQueue.h"

#include <unordered_map>

namespace fractals::persist
{

template <typename PersistClientT>
class PersistServiceImpl
{
  public:
    PersistServiceImpl(PersistEventQueue::RightEndPoint queue, PersistClientT &client);

    PersistEventQueue &getRequestQueue();
    PersistClientT& getClient();

    void pollForever();
    bool pollOnce();
    void disable();

  private:
    bool running{true};

    PersistEventQueue::RightEndPoint requestQueue;
    PersistClientT& client;
};

using PersistService = PersistServiceImpl<PersistClient>;

} // namespace fractals::network::http