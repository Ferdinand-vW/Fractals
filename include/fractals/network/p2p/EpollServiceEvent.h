#pragma once

#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/PeerFd.h"

#include <deque>
#include <epoll_wrapper/Error.h>
#include <ostream>
#include <string>
#include <variant>

namespace fractals::network::p2p
{
struct EpollError
{
    epoll_wrapper::ErrorCode errorCode;

    bool operator==(const EpollError &obj) const
    {
        return errorCode == obj.errorCode;
    }
};

std::ostream &operator<<(std::ostream &os, const EpollError &e);

struct ReadEventResponse
{
    http::PeerId peerId;
    epoll_wrapper::ErrorCode errorCode;

    bool operator==(const ReadEventResponse &obj) const
    {
        return peerId == obj.peerId && errorCode == obj.errorCode;
    }
};

std::ostream &operator<<(std::ostream &os, const ReadEventResponse &e);

struct ReadEvent
{
    PeerFd peer;
    std::vector<char> mMessage;

    bool operator==(const ReadEvent &obj) const
    {
        return peer == obj.peer && mMessage == obj.mMessage;
    }
};

std::ostream &operator<<(std::ostream &os, const ReadEvent &e);

struct Subscribe
{
    Subscribe() = default;
    Subscribe(const PeerFd &peer) : peer(peer)
    {
    }
    PeerFd peer;

    bool operator==(const Subscribe &obj) const
    {
        return peer == obj.peer;
    }
};

std::ostream &operator<<(std::ostream& os, const Subscribe &e);

struct UnSubscribe
{
    UnSubscribe() = default;
    UnSubscribe(const PeerFd &peer) : peer(peer)
    {
    }
    PeerFd peer;

    bool operator==(const UnSubscribe &obj) const
    {
        return peer == obj.peer;
    }
};

std::ostream &operator<<(std::ostream& os, const UnSubscribe &e);

struct CtlResponse
{
    CtlResponse() = default;
    CtlResponse(const PeerFd &peer, const std::string &errorMsg) : peer(peer), errorMsg(errorMsg)
    {
    }

    PeerFd peer;
    std::string errorMsg;

    operator bool() const
    {
        return errorMsg.empty();
    }

    bool operator==(const CtlResponse &obj) const
    {
        return peer == obj.peer && errorMsg == obj.errorMsg;
    }
};

std::ostream &operator<<(std::ostream& os, const CtlResponse &e);

struct WriteEvent
{
    WriteEvent() = default;
    WriteEvent(const PeerFd &peer, std::vector<char> &&message) : peer(peer), message(std::move(message))
    {
    }
    PeerFd peer;
    std::vector<char> message;

    bool operator==(const WriteEvent &obj) const
    {
        return peer == obj.peer && message == obj.message;
    }
};

std::ostream &operator<<(std::ostream& os, const WriteEvent &e);

struct WriteEventResponse
{
    WriteEventResponse() = default;
    WriteEventResponse(const PeerFd &peer, const std::string &errorMsg) : peer(peer), errorMsg(errorMsg)
    {
    }
    PeerFd peer;
    std::string errorMsg;

    operator bool() const
    {
        return errorMsg.empty();
    }

    bool operator==(const WriteEventResponse &obj) const
    {
        return peer == obj.peer && errorMsg == obj.errorMsg;
    }
};

std::ostream &operator<<(std::ostream& os, const WriteEventResponse &e);

struct ConnectionCloseEvent
{
    http::PeerId peerId;

    bool operator==(const ConnectionCloseEvent &obj) const
    {
        return peerId == obj.peerId;
    }
};

std::ostream &operator<<(std::ostream &os, const ConnectionCloseEvent &e);

struct ConnectionError
{
    http::PeerId peerId;
    epoll_wrapper::ErrorCode errorCode;

    bool operator==(const ConnectionError &obj) const
    {
        return peerId == obj.peerId && errorCode == obj.errorCode;
    }
};

std::ostream &operator<<(std::ostream &os, const ConnectionError &e);

struct Deactivate
{
    bool operator==(const Deactivate &obj) const
    {
        return true;
    }
};

std::ostream &operator<<(std::ostream &os, const Deactivate &e);

struct DeactivateResponse
{
    bool operator==(const DeactivateResponse &obj) const
    {
        return true;
    }
};

std::ostream &operator<<(std::ostream &os, const Deactivate &e);

using EpollServiceRequest = std::variant<WriteEvent, Subscribe, UnSubscribe, Deactivate>;
using EpollServiceResponse = std::variant<EpollError, ReadEventResponse, ReadEvent, WriteEventResponse, CtlResponse,
                                          ConnectionCloseEvent, ConnectionError, DeactivateResponse>;

std::ostream &operator<<(std::ostream &os, const EpollServiceRequest &pe);
std::ostream &operator<<(std::ostream &os, const EpollServiceResponse &pe);
} // namespace fractals::network::p2p