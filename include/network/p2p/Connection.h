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

typedef boost::system::error_code boost_error;
typedef std::shared_ptr<boost_error> shared_error;
typedef std::function<void(boost_error,std::shared_ptr<std::deque<char>> deq_buf)> read_callback;

class Connection : public std::enable_shared_from_this<Connection> {
    boost::asio::io_context & m_io;
    tcp::socket m_socket;
    std::mutex m_read_mutex;
    boost::asio::streambuf m_buf;
    PeerId m_peer;
    std::vector<read_callback> listeners;

    private:
        void read_message_internal(shared_error read_result,std::deque<char> &deq_buf);
        void completed_reading(boost_error error,int length);

    public:
        Connection(boost::asio::io_context &io,PeerId p);
        // Connection(Connection &&conn);

        void connect(std::function<void(boost_error)> callback);
        
        void read_messages();
        void send_message(IMessage &&m,std::function<void(boost_error,size_t)> callback);
        
        void on_receive(read_callback callback);

        void block_until(bool &cond);
        void block_until(shared_error opt);
};