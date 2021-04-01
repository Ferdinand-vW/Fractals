#include "network/p2p/Connection.h"
#include "common/utils.h"
#include "network/p2p/MessageType.h"
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/system/error_code.hpp>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>

Connection::Connection(boost::asio::io_context &io,PeerId p)
                      : m_io(io)
                      , m_socket(tcp::socket(io))
                      , m_peer(p) {};


void Connection::connect(std::function<void(boost_error)> callback) {
        auto endp = tcp::endpoint(boost::asio::ip::address::from_string(m_peer.m_ip),m_peer.m_port);
        std::cout << "endp  " << std::endl;
        // attempt to make a connection with peer
        m_socket.async_connect(endp,callback);
}

void Connection::send_message(IMessage &&m,std::function<void(boost_error,size_t)> callback) {
    boost::asio::async_write(m_socket,boost::asio::buffer(m.to_bytes_repr()),callback);
}

void Connection::read_messages() {
    //Must prevent two reads to occur at the same time
    //Otherwise one read may read the length and the other part of the first messages payload
    // std::unique_lock<std::mutex> lock(m_read_mutex);
    
    std::cout << "try to read" << std::endl;
    std::function<void(boost_error,size_t,int,int)> read_body_handler = 
        [this,&read_body_handler]
        (boost_error error, size_t size,int length,int remaining) {
            std::cout << "read body" << std::endl;
            if (size < remaining && !error) {
                boost::asio::async_read(m_socket,m_buf
                                       ,boost::asio::transfer_exactly(remaining - size)
                                       ,boost::bind(std::ref(read_body_handler)
                                       ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                                       ,length,remaining - size));            
            } else {
                completed_reading(error,length);
            }
        };

    auto read_length_handler = [&](const boost_error &err, size_t size) {
        std::cout << "read length" << std::endl;
        std::cout << err.message() << std::endl;
        std::cout << size << std::endl;
        std::deque<char> deq_buf;
        std::copy(boost::asio::buffers_begin(m_buf.data())
                 ,boost::asio::buffers_end(m_buf.data())
                 ,std::back_inserter(deq_buf));
        std::cout << "here" << std::endl;
        int length = bytes_to_int(deq_buf);
        m_buf.consume(4);
        read_body_handler(boost::asio::error::in_progress,0,length,length);
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

    auto deq_ptr = std::make_shared<std::deque<char>>(deq_buf);
    for(auto cb : listeners) {
        cb(error,deq_ptr);
    }

    read_messages(); //continue to read messages
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