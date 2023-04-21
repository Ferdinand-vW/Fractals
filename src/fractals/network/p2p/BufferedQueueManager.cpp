#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"

#include <string_view>

namespace fractals::network::p2p
{
    bool ReadMsgState::isComplete() const
    {
        // BufferedQueueManager includes length bytes which is not accounted for in mLength
        return mLength <= buffMan.size();
    }

    bool ReadMsgState::isInitialized() const
    {
        return mLength >= 0;
    }

    void ReadMsgState::initialize(uint32_t length)
    {
        mLength = length;
        buffMan.reserve(mLength);
    }

    std::vector<char>&& ReadMsgState::flush()
    {
        reset();
        return std::move(buffMan);
    }

    void ReadMsgState::reset()
    {
        mLength = -1;
    }

    std::string_view ReadMsgState::getBuffer() const
    {
        return std::basic_string_view<char>(buffMan.cbegin(),buffMan.cend());
    }

    WriteMsgState::WriteMsgState(std::vector<char>&& data) 
        : buffMan(std::move(data))
    {
        mBufferedQueueManagerView = common::string_view(buffMan.begin(), buffMan.end());
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