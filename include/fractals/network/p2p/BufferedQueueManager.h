#pragma once

#include "fractals/common/WorkQueue.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/PeerFd.h"

#include <cassert>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fractals::network::p2p
{
class ReadMsgState
{
  public:
    bool isComplete() const;
    bool isInitialized() const;
    void initialize(uint32_t length);

    template <typename Container> void append(Container &&data, int bytes)
    {
        buffer.insert(buffer.end(), data.begin(), data.begin() + bytes);
    }

    std::vector<char>&& flush();

    void reset();

    common::string_view getBuffer() const;

    int32_t getRemaining() const;

  private:
    int32_t mLength{-1};
    std::vector<char> buffer;
};

class WriteMsgState
{
  public:
    WriteMsgState(std::vector<char> &&data);

    bool isComplete() const;
    common::string_view &getBuffer();
    uint32_t remaining() const;
    // Shift should be equal to amount of data written
    void flush(uint32_t shift);

  private:
    common::string_view mBufferedQueueManagerView;
    std::vector<char> buffer;
};

class BufferedQueueManager
{
  public:
    BufferedQueueManager() = default;

    template <typename Container> bool addToReadBuffer(const PeerFd &p, Container &&data)
    {
        common::string_view view(data.begin(), data.end());
        bool hasCompletedMessage{false};
        while(!view.empty())
        {
            spdlog::info("VIEW: size={} isEmpty={} val='{}'", view.size(), view.empty(), view[0]);
            hasCompletedMessage = hasCompletedMessage | parseMessage(p, view);
        }

        return hasCompletedMessage;
    }

    bool parseMessage(const PeerFd& p, common::string_view& data)
    {
        const auto it = mReadBuffers.find(p);

        int32_t bytes = 0;
        bool complete = false;

        // Not aware of Peer yet, therefore we see handshake first
        if (it == mReadBuffers.end())
        {
            ReadMsgState m;
            uint8_t len = data[0];
            m.initialize(49 + len); // Size of HandShake message without len
            bytes = std::min(m.getRemaining(), static_cast<int32_t>(data.size()));
            m.append(data, bytes);
            mReadBuffers[p].push_back(m);

            complete = m.isComplete();
        }
        // Already aware of peer, and we've already received enough bytes to determine length
        else if (!it->second.empty() && it->second.back().isInitialized() && !it->second.back().isComplete())
        {
            bytes = std::min(it->second.back().getRemaining(), static_cast<int32_t>(data.size()));
            it->second.back().append(data, bytes);

            complete = it->second.size() > 1 || it->second.back().isComplete();
        }
        // Already aware of peer, but not yet clear how much data we are receiving
        else
        {
            if (it->second.empty() || it->second.back().isComplete())
            {
                it->second.push_back(ReadMsgState{});
            }

            ReadMsgState &m = it->second.back();

            bytes = std::min(4, static_cast<int32_t>(data.size()));
            m.append(data, bytes);

            if (m.getBuffer().size() >= 4)
            {
                auto bufView = m.getBuffer();
                const auto len = common::bytes_to_int<uint32_t>(bufView);
                m.initialize(len + 4);
            }

            complete = m.isComplete();
        }

        data.remove_prefix(bytes);
        spdlog::info("VIEW: size={} isEmpty={} bytes={}", data.size(), data.empty(), bytes);
        return complete;
    }
        
    std::deque<ReadMsgState>& getReadBuffers(const PeerFd& p)
    {
        return mReadBuffers[p];
    }

    void addToWriteBuffer(const PeerFd &p, std::vector<char> &&m)
    {
        auto it = mWriteBuffers.find(p);
        if (it == mWriteBuffers.end())
        {
            mWriteBuffers.emplace(p, WriteMsgState(std::move(m)));
        }
        else if (it->second.isComplete())
        {
            it->second = WriteMsgState(std::move(std::move(m)));
        }
    }

    const ReadMsgState *getReadBuffer(const PeerFd &p) const
    {
        return getReadBuffer(p);
    }

    ReadMsgState *getReadBuffer(const PeerFd &p)
    {
        auto it = mReadBuffers.find(p);

        if (it != mReadBuffers.end())
        {
            return &(it->second.back());
        }

        return nullptr;
    }

    WriteMsgState *getWriteBuffer(const PeerFd &p)
    {
        auto it = mWriteBuffers.find(p);

        if (it != mWriteBuffers.end())
        {
            return &(it->second);
        }

        return nullptr;
    }

  private:
    std::unordered_map<PeerFd, std::deque<ReadMsgState>> mReadBuffers;
    std::unordered_map<PeerFd, WriteMsgState> mWriteBuffers;
};

} // namespace fractals::network::p2p