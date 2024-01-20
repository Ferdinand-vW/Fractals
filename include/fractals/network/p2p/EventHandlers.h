#pragma once

#include <fractals/app/Event.h>
#include <fractals/common/utils.h>
#include <fractals/network/http/Announce.h>
#include <fractals/network/p2p/PeerEvent.h>
#include <fractals/network/p2p/Protocol.h>
#include <chrono>

namespace fractals::network::p2p
{
template <typename Caller> class PeerEventHandler
{
  public:
    PeerEventHandler(Caller *caller) : caller(caller)
    {
    }

    void handleEvent(const PeerEvent &event)
    {
        std::visit(common::overloaded{[&](const Shutdown &err)
                                      {
                                          caller->shutdown();
                                      },
                                      [&](const ConnectionDisconnected &msg)
                                      {
                                          caller->process(msg);
                                      },
                                      [&](const ConnectionEstablished &msg)
                                      {
                                          caller->process(msg);
                                      },
                                      [&](const Message &msg)
                                      {
                                          handleMessage(msg);
                                      }},
                   event);
    }

    void handleMessage(const Message &event)
    {
        // clang-format off
        std::visit(common::overloaded{
            [&](const SerializeError &error) {},
            [&](const auto &msg) 
            {
                using T = std::decay_t<decltype(msg)>;
                if constexpr (!std::is_same_v<T, SerializeError>)
                {
                    const auto res = caller->template forwardToPeer<T>(event.peer, msg);
                    switch (res.first)
                    {
                    case ProtocolState::ERROR:
                        caller->shutdown();
                        break;

                    case ProtocolState::HASH_CHECK_FAIL:
                        caller->disconnectPeer(event.peer);
                        break;

                    case ProtocolState::CLOSED:
                        caller->disconnectPeer(event.peer);
                        break;

                    case ProtocolState::COMPLETE:
                        caller->peerCompletedTorrent(event.peer, res.second);
                        break;

                    case ProtocolState::OPEN:
                        break;
                    }
                }
            }},
        event.message);
        // clang-format on
    }

  private:
    Caller *caller;
};

template <typename Caller> class DiskEventHandler
{
  public:
    DiskEventHandler(Caller *caller) : caller(caller)
    {
    }

    void handleEvent(const disk::DiskResponse &event)
    {
        std::visit(common::overloaded{[this](const auto &addTorr)
                                      {
                                          caller->process(addTorr);
                                      }},
                   event);
    }

  private:
    Caller *caller;
};

template <typename Caller> class AnnounceEventHandler
{
  public:
    AnnounceEventHandler(Caller *caller) : caller(caller)
    {
    }

    void handleEvent(const http::Announce &event)
    {
        caller->process(event);
    }

  private:
    Caller *caller;
};

template <typename Caller> class PersistEventHandler
{
  public:
    PersistEventHandler(Caller *caller) : caller(caller)
    {
    }

    void handleEvent(const persist::PersistResponse &event)
    {
        std::visit(common::overloaded{[&](const auto &resp)
                                      {
                                          caller->process(resp);
                                      }},
                   event);
    }

    void handleEvent(const persist::AppPersistResponse &event)
    {
        std::visit(common::overloaded{[&](const auto &resp)
                                      {
                                          caller->process(resp);
                                      }},
                   event);
    }

  private:
    Caller *caller;
};

template <typename Caller> class AppEventHandler
{
  public:
    AppEventHandler(Caller *caller) : caller(caller)
    {
    }

    void handleEvent(const app::RequestFromApp &event)
    {
        std::visit(common::overloaded{[this](const auto &addTorr)
                                      {
                                          caller->process(addTorr);
                                      }},
                   event);
    }

    void handleMessage(const Message &event)
    {
    }

  private:
    Caller *caller;
};
} // namespace fractals::network::p2p