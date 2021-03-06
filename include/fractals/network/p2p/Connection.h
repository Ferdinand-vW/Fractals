#pragma once

#include <cstddef>
#include <deque>
#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/sources/logger.hpp>

#include "fractals/network/p2p/ConcurrentTimer.h"
#include "fractals/network/http/Peer.h"

namespace fractals::network::p2p {

    using namespace boost::asio;
    using ip::tcp;

    class IMessage;

    typedef boost::system::error_code boost_error;
    typedef std::shared_ptr<boost_error> shared_error;
    typedef std::function<void(const boost_error&,int length,std::deque<char> &&deq_buf)> read_callback;

    class Connection : public std::enable_shared_from_this<Connection> {
        boost::asio::io_context & m_io;
        ConcurrentTimer m_timer;

        boost::asio::streambuf m_buf;
        http::PeerId m_peer;
        std::vector<read_callback> listeners;
        read_callback m_handshake_callback; 

        boost::log::sources::logger_mt &m_lg;

        private:
            void read_message_body(const boost_error&error,size_t size,int length, int remaining);
            void read_handshake_body(const boost_error&error,size_t size,unsigned char pstrlen,int length, int remaining);

        public:
            tcp::socket m_socket;
            Connection(boost::asio::io_context &io,http::PeerId p);

            boost::asio::io_context& get_io() {
                return m_io;
            }

            ConcurrentTimer& get_timer() {
                return m_timer;
            }
            // Connection(Connection &&conn);

            void connect(std::function<void(const boost_error&)>);
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

}