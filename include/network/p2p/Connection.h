#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <deque>
#include <memory>
#include <future>
#include <iostream>

#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/Response.h"

using namespace boost::asio;
using ip::tcp;

typedef boost::system::error_code boost_error;
typedef std::shared_ptr<boost_error> shared_error;
typedef std::function<void(const boost_error&,int length,std::deque<char> &&deq_buf)> read_callback;

class Connection : public std::enable_shared_from_this<Connection> {
    boost::asio::io_context & m_io;
    boost::asio::deadline_timer m_timer;

    boost::asio::streambuf m_buf;
    PeerId m_peer;
    std::vector<read_callback> listeners;
    read_callback m_handshake_callback; 

    private:
        void read_message_body(const boost_error&error,size_t size,int length, int remaining);
        void read_handshake_body(const boost_error&error,size_t size,unsigned char pstrlen,int length, int remaining);

    public:
        tcp::socket m_socket;
        Connection(boost::asio::io_context &io,PeerId p);

        boost::asio::io_context& get_io() {
            return m_io;
        }

        boost::asio::deadline_timer& get_timer() {
            return m_timer;
        }
        // Connection(Connection &&conn);

        void connect(std::chrono::seconds timeout,std::function<void(const boost_error&)>);
        bool is_open();
        void cancel();

        void read_messages();
        void read_handshake();
        void write_message(std::unique_ptr<IMessage> m,std::function<void(const boost_error&,size_t)> callback);
    
        void on_receive(read_callback callback);
        void on_handshake(read_callback callback);

        void block_until(bool &cond);
        void block_until(shared_error opt);
        void timed_block_until(std::optional<boost_error> &result,std::optional<boost_error> &timer);
};