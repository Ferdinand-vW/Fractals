#include "fractals/network/p2p/BufferManager.h"
#include "fractals/network/http/Peer.h"

#include <string_view>

namespace fractals::network::p2p
{
    ReadMsgState::ReadMsgState()
    {
        mLengthBuffer.reserve(4);
    }

    bool ReadMsgState::isComplete() const
    {
        return mLength == mBuffer.size();
    }

    bool ReadMsgState::isInitialized() const
    {
        return mLength >= 0;
    }

    void ReadMsgState::initialize(uint32_t length)
    {
        mLength = length;
        mBuffer.reserve(mLength);
    }

    void ReadMsgState::append(const std::vector<char>& data, uint8_t skip)
    {
        mBuffer.insert(mBuffer.end(), data.begin() + skip, data.end());
    }

    void ReadMsgState::appendLength(const std::vector<char> &data, uint32_t take)
    {
        mLengthBuffer.insert(mLengthBuffer.end(), data.begin(), data.begin() + take);
    }

    uint32_t ReadMsgState::lengthBufferSize() const
    {
        return mLengthBuffer.size();
    }

    void ReadMsgState::reset()
    {
        mLength = -1;
    }

    WriteMsgState::WriteMsgState(std::vector<char>&& data) : mBuffer(std::move(data)) {}

    bool WriteMsgState::isComplete() const
    {
        return mPos > mBuffer.size();
    }

    const std::string_view WriteMsgState::getBuffer() const
    {
        return std::basic_string_view<char>(mBuffer.cbegin(),mBuffer.cend());
    }

    void WriteMsgState::moveWritePointer(uint32_t shift)
    {
        mPos += shift;
    }
}