#include "network/p2p/Connection.h"
#include "common/utils.h"
#include "network/p2p/MessageType.h"
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
                      , m_socket(tcp::socket(io))
                      , m_peer(p) {};


FutureResponse Connection::connect(std::chrono::seconds timeout) {
    auto endp = tcp::endpoint(boost::asio::ip::address::from_string(m_peer.m_ip),m_peer.m_port);
    // attempt to make a connection with peer
    auto fut = m_socket.async_connect(endp,boost::asio::use_future);
    auto fut_status = fut.wait_for(timeout);

    return FutureResponse { std::make_unique<std::deque<char>>(), fut_status };
}

bool Connection::is_open() {
    return m_socket.is_open();
}

void Connection::cancel() {
    m_socket.cancel();
}

void Connection::write_message(std::unique_ptr<IMessage> m,std::function<void(const boost_error&,size_t)> callback) {
    std::cout << bytes_to_hex(m->to_bytes_repr()) << std::endl;
    boost::asio::async_write(m_socket,boost::asio::buffer(m->to_bytes_repr()),callback);
}

void Connection::read_message_body(const boost_error& error,size_t size,int length,int remaining) {
    if (size < remaining && !error) {
        boost::asio::async_read(m_socket,m_buf
                                ,boost::asio::transfer_exactly(remaining - size)
                                ,boost::bind(&Connection::read_message_body,this
                                ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                                ,length,remaining - size));            
    } else {
        completed_reading(error,length);
    }
}

void Connection::read_messages() {
    
    auto read_length_handler = [&](const boost_error &err, size_t size) {
        // Abort on error
        if (err) {
            std::cout << "[Connection] " << err.message() << std::endl;
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

void Connection::completed_reading(boost_error error,int length) {
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

FutureResponse Connection::timed_blocking_receive(std::chrono::seconds timeout) {
    boost::asio::streambuf buf;
    std::future<size_t> fut_length = boost::asio::async_read(m_socket,buf,boost::asio::transfer_exactly(1),boost::asio::use_future);
    auto fut_status = fut_length.wait_for(timeout);
    
    if(fut_status != std::future_status::ready) {
        return FutureResponse { std::make_unique<std::deque<char>>(), fut_status };
    }

    std::deque<char> deq_buf;
    std::copy(boost::asio::buffers_begin(buf.data())
             ,boost::asio::buffers_end(buf.data())
             ,std::back_inserter(deq_buf));

    int length = bytes_to_int(deq_buf) + 48;
    buf.consume(1);

    int remaining = length;
    while(remaining > 0) {
        auto fut_body = boost::asio::async_read(m_socket,buf,boost::asio::transfer_exactly(length),boost::asio::use_future);

        auto fut_status = fut_body.wait_for(timeout);
        if(fut_status != std::future_status::ready) {
            return FutureResponse { std::make_unique<std::deque<char>>(deq_buf),fut_status};
        } else {
            size_t size = fut_body.get();
            remaining -= size;
        }
    }

    std::copy(boost::asio::buffers_begin(buf.data())
             ,boost::asio::buffers_end(buf.data())
             ,std::back_inserter(deq_buf));

    buf.consume(length);

    return FutureResponse { std::make_unique<std::deque<char>>(deq_buf), std::future_status::ready };
}

void Connection::on_receive(read_callback callback) {
    listeners.push_back(callback);
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