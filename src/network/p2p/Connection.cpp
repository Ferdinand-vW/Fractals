#include "network/p2p/Connection.h"
#include "common/utils.h"
#include "common/logger.h"
#include "network/p2p/MessageType.h"
#include "network/p2p/Message.h"
#include <bits/c++config.h>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <deque>
#include <exception>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>

Connection::Connection(boost::asio::io_context &io,PeerId p)
                      : m_io(io)
                      , m_timer(ConcurrentTimer(io))
                      , m_socket(tcp::socket(io))
                      , m_peer(p)
                      , m_lg(logger::get()) {};


void Connection::connect(std::function<void(const boost_error&)> callback) {
    auto endp = tcp::endpoint(boost::asio::ip::address::from_string(m_peer.m_ip),m_peer.m_port);
    // attempt to make a connection with peer
    m_socket.async_connect(endp,callback);
}

bool Connection::is_open() {
    return m_socket.is_open();
}

void Connection::cancel() {
    // gracefully cancel outstanding operations before closing socket
    try {
        m_timer.cancel();
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        m_socket.close();
    }
    catch(std::exception e) {
        BOOST_LOG(m_lg) << "[Connection] exception on socket closure: " << e.what();
    }
}

void Connection::write_message(std::unique_ptr<IMessage> m,std::function<void(const boost_error&,size_t)> callback) {
    boost::asio::async_write(m_socket,boost::asio::buffer(m->to_bytes_repr()),callback);
}

void Connection::read_message_body(const boost_error& error,size_t size,int length,int remaining) {
    if (size < remaining && !error) {
        boost::asio::async_read(m_socket,m_buf
                                ,boost::asio::transfer_exactly(remaining - size)
                                ,boost::bind(&Connection::read_message_body,shared_from_this()
                                ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                                ,length,remaining - size));            
    } else {
        if(error) {
            BOOST_LOG(m_lg) << "[Connection] body handler " << error.message();
        }

        std::deque<char> deq_buf;

        std::copy(boost::asio::buffers_begin(m_buf.data())
                ,boost::asio::buffers_end(m_buf.data())
                ,std::back_inserter(deq_buf));
        m_buf.consume(length);

        for(auto cb : listeners) {
            cb(error,length,std::move(deq_buf));
        }

        read_messages(); //continue to read messages
    }
}

void Connection::read_messages() {
    auto read_length_handler = [&](const boost_error &err, size_t size) {
        // Abort on error
        if (err) {
            BOOST_LOG(m_lg) << "[Connection] length handler: " << err.message();
            return;
        }
 
        std::deque<char> deq_buf;
        std::copy(boost::asio::buffers_begin(m_buf.data())
                 ,boost::asio::buffers_end(m_buf.data())
                 ,std::back_inserter(deq_buf));
        
        int length = bytes_to_int(deq_buf);
        m_buf.consume(4); //TODO: Don't assume we've already read 4 bytes

        read_message_body(boost_error(),0,length,length);
    };

    boost::asio::async_read(m_socket,m_buf
                            ,boost::asio::transfer_exactly(4)
                            ,boost::bind<void>(read_length_handler
                            ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));  
}

void Connection::read_handshake_body(const boost_error& error,size_t size,unsigned char pstrlen,int length,int remaining) {
    if (size < remaining && !error) {
        boost::asio::async_read(m_socket,m_buf
                                ,boost::asio::transfer_exactly(remaining - size)
                                ,boost::bind(&Connection::read_handshake_body,shared_from_this()
                                ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                                ,pstrlen,length,remaining - size));            
    } else {
        std::deque<char> deq_buf;

        std::copy(boost::asio::buffers_begin(m_buf.data())
                ,boost::asio::buffers_end(m_buf.data())
                ,std::back_inserter(deq_buf));
        m_buf.consume(length);
        //pass the pstrlen which is necessary to construct a handshake message
        m_handshake_callback(error,pstrlen,std::move(deq_buf));
    }
}

void Connection::read_handshake() {
    auto read_length_handler = [&](const boost_error &err, size_t size) {
        // Abort on error
        if (err) {
            BOOST_LOG(m_lg) << "[Connection] " << err.message();
            return;
        }


        std::deque<char> deq_buf;
        std::copy(boost::asio::buffers_begin(m_buf.data())
                 ,boost::asio::buffers_end(m_buf.data())
                 ,std::back_inserter(deq_buf));
        
        unsigned char pstrlen = bytes_to_int(deq_buf);
        int length = pstrlen + 48;
        m_buf.consume(1);

        read_handshake_body(boost_error(),0,pstrlen,length,length);
    };

    boost::asio::async_read(m_socket,m_buf,boost::asio::transfer_exactly(1),
                                         boost::bind<void>(read_length_handler
                                         ,boost::asio::placeholders::error
                                         ,boost::asio::placeholders::bytes_transferred));
}

void Connection::on_receive(read_callback callback) {
    listeners.push_back(callback);
}

void Connection::on_handshake(read_callback callback) {
    m_handshake_callback = callback;
}

void Connection::block_until(bool &cond) {
    while(!cond) {
        m_io.run_one();
    }
}

void Connection::block_until(shared_error opt) {
    while (opt == nullptr) {
        m_io.run_one();
    }
}

void Connection::timed_block_until(std::optional<boost_error> &result, std::optional<boost_error> &timer) {
    while (!result && !timer) {
        m_io.run_one();
    }
}