#include "fractals/network/p2p/BufferManager.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"

#include <string_view>

namespace fractals::network::p2p
{
    bool ReadMsgState::isComplete() const
    {
        // Buffer includes length bytes which is not accounted for in mLength
        return mLength <= mBuffer.size();
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

    void ReadMsgState::append(const std::vector<char>& data)
    {
        mBuffer.insert(mBuffer.end(), data.begin(), data.end());
    }

    void ReadMsgState::reset()
    {
        mLength = -1;
    }

    std::string_view ReadMsgState::getBuffer() const
    {
        return std::basic_string_view<char>(mBuffer.cbegin(),mBuffer.cend());
    }

    WriteMsgState::WriteMsgState(std::vector<char>&& data) 
        : mBuffer(std::move(data))
    {
        mBufferView = common::string_view(mBuffer.begin(), mBuffer.end());
    }

    bool WriteMsgState::isComplete() const
    {
        return mBufferView.empty();
    }

    std::string_view& WriteMsgState::getBuffer()
    {
        return mBufferView;
    }

    void WriteMsgState::flush(uint32_t shift)
    {
        mBufferView.remove_prefix(shift);
    }
}