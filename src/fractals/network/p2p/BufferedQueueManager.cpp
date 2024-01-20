#include <fractals/network/p2p/BufferedQueueManager.h>
#include <fractals/common/utils.h>
#include <fractals/network/http/Peer.h>

#include <string_view>

namespace fractals::network::p2p
{
    bool ReadMsgState::isComplete() const
    {
        // BufferedQueueManager includes length bytes which is not accounted for in mLength
        return mLength <= buffer.size();
    }

    bool ReadMsgState::isInitialized() const
    {
        return mLength >= 0;
    }

    void ReadMsgState::initialize(uint32_t length)
    {
        mLength = length;
        buffer.reserve(mLength);
    }

    std::vector<char>&& ReadMsgState::flush()
    {
        reset();
        return std::move(buffer);
    }

    void ReadMsgState::reset()
    {
        mLength = -1;
    }

    std::string_view ReadMsgState::getBuffer() const
    {
        return std::string_view(buffer.cbegin(),buffer.cend());
    }

    int32_t ReadMsgState::getRemaining() const
    {
        return mLength - buffer.size();
    }

    WriteMsgState::WriteMsgState(std::vector<char>&& data) 
        : buffer(std::move(data))
    {
        mBufferedQueueManagerView = std::string_view(buffer.begin(), buffer.end());
    }

    bool WriteMsgState::isComplete() const
    {
        return mBufferedQueueManagerView.empty();
    }

    std::string_view& WriteMsgState::getBuffer()
    {
        return mBufferedQueueManagerView;
    }

    uint32_t WriteMsgState::remaining() const
    {
        return mBufferedQueueManagerView.size();
    }

    void WriteMsgState::flush(uint32_t shift)
    {
        mBufferedQueueManagerView.remove_prefix(shift);
    }
}