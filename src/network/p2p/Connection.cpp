#include "network/p2p/Connection.h"
#include "common/utils.h"
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
#include <mutex>

Connection::Connection(boost::asio::io_context &io)
                      : m_io(io)
                      , m_socket(tcp::socket(io)) {};

Connection::Connection(Connection &&other)
                      : m_io(other.m_io)
                      , m_socket(std::move(other.m_socket)) {};

void Connection::f_timed(std::function<void(std::optional<boost_error>&)> f
                        ,std::function<void(boost_error)> callback
                        ,boost::posix_time::seconds timeout) {
    std::function<void(std::optional<boost_error>&,std::deque<char>&)> f2 = [f](auto err,auto _deq) {
        return f(err);
    };

    std::function<void(boost_error,std::shared_ptr<std::deque<char>>)> callback2 = [callback](auto err, auto _deq) {
        return callback(err);
    };

    return f_timed(f2,callback2,timeout);
}
void Connection::f_timed(std::function<void(std::optional<boost_error>&,std::deque<char>&)> f
                        ,std::function<void(boost_error,std::shared_ptr<std::deque<char>>)> callback
                        ,boost::posix_time::seconds timeout) {
    boost::asio::deadline_timer timer(m_io);

    std::optional<boost_error> timer_result;
    std::optional<boost_error> read_result;

    // Assume negative timer means infinite timer
    if(timeout.is_negative()) { timer.expires_from_now(boost::posix_time::pos_infin); }
    else                     { timer.expires_from_now(timeout); }
    timer.async_wait([&timer_result](const boost_error &error) { timer_result = error; });

    std::deque<char> deq;
    f(read_result,deq);

    // looping over event handler until connection has been made or we time out
    bool handled = false;
    while(!handled) {
        m_io.run_one(); // blocks

        // if one succeeds we can cancel the other
        if(read_result) {
            timer.cancel();
            handled = true;
        } else if (timer_result) {
            m_socket.cancel();
            handled = true;
        }
    }

    auto deq_ptr = std::make_shared<std::deque<char>>(deq);

    if(timer_result) { callback(timer_result.value(),std::move(deq_ptr)); }
    if(read_result)  { callback(read_result.value(),std::move(deq_ptr)); }
}

void Connection::connect(PeerId p,std::function<void(boost_error)> callback) {
    auto endp = tcp::endpoint(boost::asio::ip::address::from_string(p.m_ip),p.m_port);

    auto connect_f = [this,endp,callback](auto &read_result) {
        // attempt to make a connection with peer
        try {
            m_socket.async_connect(endp,[&read_result](const boost_error &error) { read_result = error; });
        }
        catch(std::exception &error) {
            read_result = boost::asio::error::connection_refused;
        }
    };

    f_timed(connect_f, callback,boost::posix_time::seconds(10));

}

void Connection::send_message(IMessage &&m,std::function<void(boost_error,size_t)> callback) {
    boost::asio::async_write(m_socket,boost::asio::buffer(m.to_bytes_repr()),callback);
}

void Connection::read_message_internal(std::optional<boost_error> &read_result,std::deque<char> &deq_buf) {
    //Must prevent two reads to occur at the same time
    //Otherwise one read may read the length and the other part of the first messages payload
    std::unique_lock<std::mutex> lock(m_read_mutex);
    
    boost::asio::streambuf buf;

    std::function<void(boost_error,size_t,int,int)> read_body_handler = 
        [this,&read_result,&buf,&read_body_handler]
        (boost_error error, size_t size,int length,int remaining) {
            if (size < remaining) {
                boost::asio::async_read(m_socket,buf
                                       ,boost::asio::transfer_exactly(remaining - size)
                                       ,boost::bind(std::ref(read_body_handler)
                                       ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                                       ,length,remaining - size));            
            } else {
                read_result = error;
            }
        };

    auto read_length_handler = [&buf,&read_body_handler](const boost_error &err, size_t size) {

        std::deque<char> deq_buf;
        std::copy(boost::asio::buffers_begin(buf.data())
                 ,boost::asio::buffers_end(buf.data())
                 ,std::back_inserter(deq_buf));
        
        int length = bytes_to_int(deq_buf);
        buf.consume(4);
        read_body_handler(boost::asio::error::in_progress,0,length,length);
    };

    boost::asio::async_read(m_socket,buf
                            ,boost::asio::transfer_exactly(4)
                            ,boost::bind<void>(read_length_handler
                            ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));  

    std::copy(boost::asio::buffers_begin(buf.data())
                ,boost::asio::buffers_end(buf.data())
                ,std::back_inserter(deq_buf));
}

void Connection::read_message(std::function<void (boost_error, std::shared_ptr<std::deque<char>>)> callback) {
    //f_timed(boost::bind(&Connection::read_message_internal,shared_from_this()),callback);
}
void Connection::read_message_timed(std::function<void(boost_error,std::shared_ptr<std::deque<char>>)> callback) {
    auto func = [this](std::optional<boost_error>& read_result,std::deque<char>& deq_buf) {
        read_message_internal(read_result,deq_buf);
    };

    f_timed(func,callback,boost::posix_time::seconds(10));
}

void Connection::block_until(bool &cond) {
    while(!cond) {
        m_io.run_one();
    }
}