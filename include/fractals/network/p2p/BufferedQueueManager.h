#pragma once

#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/Event.h"
#include "fractals/network/p2p/WorkQueue.h"
#include "fractals/common/utils.h"

#include <string_view>
#include <vector>
#include <span>
#include <unordered_map>

namespace fractals::network::p2p
{
    class ReadMsgState
    {
        public:
            bool isComplete() const;
            bool isInitialized() const;
            void initialize(uint32_t length);
            
            template <typename Container>
            void append(Container&& data)
            {
                mBufferedQueueManager.insert(mBufferedQueueManager.end(), data.begin(), data.end());
            }

            void reset();

            common::string_view getBufferedQueueManager() const;

        private:
            int32_t mLength{-1};
            std::vector<char> mBufferedQueueManager;
    };

    class WriteMsgState
    {
        public:
            WriteMsgState(std::vector<char>&& data);

            bool isComplete() const;
            common::string_view& getBufferedQueueManager();
            // Shift should be equal to amount of data written
            void flush(uint32_t shift);

        private:
            common::string_view mBufferedQueueManagerView;
            std::vector<char> mBufferedQueueManager;
    };

    template <typename MsgQueue>
    class BufferedQueueManagerImpl
    {
        public:
            BufferedQueueManagerImpl(MsgQueue &mq) : mMsgQueue(mq) {}

            template <typename Container>
            void addToReadBuffers(const http::PeerId& p, Container&& data)
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

                    maybePublish(p, m);
                }
                // Already aware of peer, and we've already received enough bytes to determine length
                else if (it->second.isInitialized())
                {
                    it->second.append(data);
                    maybePublish(p, it->second);
                }
                // Already aware of peer, but not yet clear how much data we are receiving
                else
                {
                    ReadMsgState &m = it->second;

                    m.append(data);

                    if (m.getBufferedQueueManager().size() >= 4)
                    {
                        auto bufView = m.getBufferedQueueManager();
                        const auto len = common::bytes_to_int<uint32_t>(bufView);
                        m.initialize(len);
                    }

                    maybePublish(p, m);
                }
            }

            void maybePublish(const http::PeerId& p, ReadMsgState& m)
            {
                if (m.isComplete())
                {
                    mMsgQueue.push(ReceiveEvent{p, mEncoder.decode(m.getBufferedQueueManager())});
                    mReadBuffers[p] = ReadMsgState();
                }
            }

            void publishToQueue(const PeerEvent& pe)
            {
                mMsgQueue.push(pe);
            }

            void addToWriteBufferedQueueManager(const http::PeerId& p, const BitTorrentMessage& m)
            {
                mWriteBuffers.emplace(p, WriteMsgState(mEncoder.encode(m)));
            }

            void flush(const http::PeerId& p, uint32_t bytes)
            {
                auto it = mWriteBuffers.find(p);

                if (it != mWriteBuffers.end())
                {
                    it->second.flush(bytes);
                }
            }

            const ReadMsgState* getReadBuffer(const http::PeerId& p) const
            {
                auto it = mReadBuffers.find(p);

                if (it != mReadBuffers.end())
                {
                    return &(it->second);
                }

                return nullptr;
            }

            WriteMsgState* getWriteBuffer(const http::PeerId& p)
            {
                auto it = mWriteBuffers.find(p);

                if (it != mWriteBuffers.end())
                {
                    return &(it->second);
                }

                return nullptr;
            }


            void notifyWrittenBytes(const http::PeerId &p, uint32_t numBytes)
            {
                mWriteBuffers[p].flush(numBytes);
            }

        private:

            BitTorrentEncoder mEncoder;
            MsgQueue &mMsgQueue;
            std::unordered_map<http::PeerId, ReadMsgState> mReadBuffers;
            std::unordered_map<http::PeerId, WriteMsgState> mWriteBuffers;
    };

    using BufferedQueueManager = BufferedQueueManagerImpl<WorkQueue>;
}