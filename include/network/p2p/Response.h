#pragma once

#include <boost/system/error_code.hpp>
#include "network/p2p/Message.h"

struct Response {
    std::unique_ptr<IMessage> m_message;
    boost::system::error_code error;
};