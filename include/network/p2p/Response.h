#pragma once

#include <boost/system/error_code.hpp>
#include <future>
#include "network/p2p/Message.h"

struct Response {
    std::unique_ptr<IMessage> m_message;
    boost::system::error_code error;
};

struct FutureResponse {
    std::unique_ptr<std::deque<char>> m_data;
    std::future_status m_status;
};