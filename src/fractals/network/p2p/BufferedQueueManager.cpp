#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"

#include <string_view>

namespace fractals::network::p2p
{
    bool ReadMsgState::isComplete() const
    {
        // BufferedQueueManager includes length bytes which is not accounted for in mLength
        return mLength <= mBufferedQueueManager.size();
    }

    bool ReadMsgState::isInitialized() const
    {
        return mLength >= 0;
    }

    void ReadMsgState::initialize(uint32_t length)
    {
        mLength = length;
        mBufferedQueueManager.reserve(mLength);
    }

    void ReadMsgState::reset()
    {
        mLength = -1;
    }

    std::string_view ReadMsgState::getBufferedQueueManager() const
    {
        return std::basic_string_view<char>(mBufferedQueueManager.cbegin(),mBufferedQueueManager.cend());
    }

    WriteMsgState::WriteMsgState(std::vector<char>&& data) 
        : mBufferedQueueManager(std::move(data))
    {
        mBufferedQueueManagerView = common::string_view(mBufferedQueueManager.begin(), mBufferedQueueManager.end());
    }

    bool WriteMsgState::isComplete() const
    {
        return mBufferedQueueManagerView.empty();
    }

    std::string_view& WriteMsgState::getBufferedQueueManager()
    {
        return mBufferedQueueManagerView;
    }

    void WriteMsgState::flush(uint32_t shift)
    {
        mBufferedQueueManagerView.remove_prefix(shift);
    }
}