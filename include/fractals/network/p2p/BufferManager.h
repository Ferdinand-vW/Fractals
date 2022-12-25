#pragma once

#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
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
            ReadMsgState();

            bool isComplete() const;
            bool isInitialized() const;
            void initialize(uint32_t length);
            void append(const std::vector<char>& data, uint8_t skip = 0);
            void appendLength(const std::vector<char>& data, uint32_t take);
            uint32_t lengthBufferSize() const;
            void reset();

            const common::string_view getBuffer() const;

        private:
            int32_t mLength{-1};
            std::vector<char> mLengthBuffer;
            std::vector<char> mBuffer;
    };

    class WriteMsgState
    {
        public:
            WriteMsgState(std::vector<char>&& data);

            bool isComplete() const;
            const common::string_view getBuffer() const;
            // Shift should be equal to amount of data written
            void moveWritePointer(uint32_t shift);

        private:
            uint32_t mPos{0};
            std::vector<char> mBuffer;
    };

    template <typename MsgQueue>
    class BufferManagerImpl
    {
        public:
            BufferManagerImpl(MsgQueue &mq) : mMsgQueue(mq) {}

            void addToReadBuffer(const http::PeerId& p, std::vector<char>&& data)
            {
                const auto it = mReadBuffers.find(p);
                
                // Not aware of Peer yet, therefore we see handshake first
                if (it == mReadBuffers.end())
                {
                    ReadMsgState m;
                    uint8_t len = data[0];
                    m.initialize(49 + len); // Size of HandShake message
                    m.append(data, 1);
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
                    uint32_t curLenSize = m.lengthBufferSize();
                    if (curLenSize + data.size() < 4) // Not enough bytes for length
                    {
                        m.appendLength(data, data.size());
                    }
                    else
                    {
                        m.appendLength(data, 4 - curLenSize);
                        m.append(data, 4 - curLenSize);
                    }

                    maybePublish(p, m);
                }
            }

            void maybePublish(const http::PeerId& p, ReadMsgState& m)
            {
                if (m.isComplete())
                {
                    mMsgQueue.push(mEncoder.decode(m.getBuffer()));
                    mReadBuffers[p] = ReadMsgState();
                }
            }

            void addToWriteBuffer(const http::PeerId& p, const BitTorrentMessage& m)
            {
                mWriteBuffers[p] = WriteMsgState(mEncoder.encode(m));
            }

            common::string_view getWriteBufferFor(const http::PeerId& p)
            {
                return mWriteBuffers.at(p).getBuffer();
            }

            void notifyWrittenBytes(const http::PeerId &p, uint32_t numBytes)
            {
                mWriteBuffers[p].moveWritePointer(numBytes);
            }

        private:

            BitTorrentEncoder mEncoder;
            MsgQueue &mMsgQueue;
            std::unordered_map<http::PeerId, ReadMsgState> mReadBuffers;
            std::unordered_map<http::PeerId, WriteMsgState> mWriteBuffers;
    };

    using ReadBufferManager = BufferManagerImpl<WorkQueue>;
}