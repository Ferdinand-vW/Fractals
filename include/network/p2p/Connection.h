#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/system/error_code.hpp>
#include <deque>
#include <memory>

#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"

using namespace boost::asio;
using ip::tcp;

typedef boost::system::error_code error_code;

class Connection : std::enable_shared_from_this<Connection> {
    boost::asio::io_context & m_io;
    boost::asio::ip::tcp::socket m_socket;
    std::mutex m_read_mutex;

    private:
        void f_timed(std::function<void(std::optional<error_code>&)> f
                    ,std::function<void(error_code)> callback);
        void f_timed(std::function<void(std::optional<error_code>&,std::deque<char> &)> f
                    ,std::function<void(error_code,std::deque<char>&&)> callback);

        void read_message_internal(std::optional<boost::system::error_code> &read_result,std::deque<char> &deq_buf);

    public:
        Connection(boost::asio::io_context &io);

        void connect(PeerId p,std::function<void(error_code)> callback);
        
        void send_message(IMessage &&m,std::function<void(error_code,size_t)> callback);
        void send_message_timed(IMessage &&m,std::function<void(error_code,size_t)> callback);
        
        void read_message(std::function<void(error_code,std::deque<char> &&deq_buf)> callback);
        void read_message_timed(std::function<void(error_code,std::deque<char> &&deq_buf)> callback);
};