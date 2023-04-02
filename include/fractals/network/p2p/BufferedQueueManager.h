#pragma once

#include "fractals/common/WorkQueue.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/PeerEventQueue.h"
#include "fractals/network/p2p/PeerFd.h"

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

    template <typename Container> void append(Container &&data)
    {
        mBufferedQueueManager.insert(mBufferedQueueManager.end(), data.begin(), data.end());
    }

    void reset();

    common::string_view getBuffer() const;

  private:
    int32_t mLength{-1};
    std::vector<char> mBufferedQueueManager;
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
    std::vector<char> mBufferedQueueManager;
};

template <typename MsgQueue> class BufferedQueueManagerImpl
{
  public:
    BufferedQueueManagerImpl(MsgQueue &mq) : mMsgQueue(mq)
    {
    }

    template <typename Container> void readPeerData(const PeerFd &p, Container &&data)
    {
        const auto it = mReadBuffers.find(p);

        // Not aware of Peer yet, therefore we see handshake first
        if (it == mReadBuffers.end())
        {
            ReadMsgState m;
            uint8_t len = data[0];
            m.initialize(49 + len - 1); // Size of HandShake message without len
            m.append(data);
            mReadBuffers[p] = m;

            publishOnComplete(p, m);
        }
        // Already aware of peer, and we've already received enough bytes to determine length
        else if (it->second.isInitialized())
        {
            it->second.append(data);
            publishOnComplete(p, it->second);
        }
        // Already aware of peer, but not yet clear how much data we are receiving
        else
        {
            ReadMsgState &m = it->second;

            m.append(data);

            if (m.getBuffer().size() >= 4)
            {
                auto bufView = m.getBuffer();
                const auto len = common::bytes_to_int<uint32_t>(bufView);
                m.initialize(len);
            }

            publishOnComplete(p, m);
        }
    }

    void sendToPeer(const PeerFd &p, const BitTorrentMessage &m)
    {
        auto it = mWriteBuffers.find(p);
        if (it == mWriteBuffers.end())
        {
            mWriteBuffers.emplace(p, WriteMsgState(mEncoder.encode(m)));
        }
        else if (it->second.isComplete())
        {
            it->second = WriteMsgState(std::move(mEncoder.encode(m)));
        }
    }

    const ReadMsgState *getReadBuffer(const PeerFd &p) const
    {
        auto it = mReadBuffers.find(p);

        if (it != mReadBuffers.end())
        {
            return &(it->second);
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

    void publish(const PeerEvent &pe)
    {
        mMsgQueue.push(pe);
    }

  private:
    void publishOnComplete(const PeerFd &p, ReadMsgState &m)
    {
        if (m.isComplete())
        {
            mMsgQueue.push(ReceiveEvent{p.getId(), mEncoder.decode(m.getBuffer())});
            mReadBuffers[p] = ReadMsgState();
        }
    }

    void clearWriteBuffer(const PeerFd &p, uint32_t bytes)
    {
        auto it = mWriteBuffers.find(p);

        if (it != mWriteBuffers.end())
        {
            it->second.flush(bytes);
        }
    }

  private:
    BitTorrentEncoder mEncoder;
    MsgQueue &mMsgQueue;
    std::unordered_map<PeerFd, ReadMsgState> mReadBuffers;
    std::unordered_map<PeerFd, WriteMsgState> mWriteBuffers;
};

using BufferedQueueManager = BufferedQueueManagerImpl<PeerEventQueue>;
} // namespace fractals::network::p2p