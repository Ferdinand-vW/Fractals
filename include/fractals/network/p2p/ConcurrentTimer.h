#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <mutex>

namespace fractals::network::p2p {

    class ConcurrentTimer {
        boost::asio::deadline_timer m_timer;
        bool is_enabled = true;
        std::mutex m_mutex;

        public:
            ConcurrentTimer(boost::asio::io_context &io) : m_timer(boost::asio::deadline_timer(io)) {};

            void disable() {
                is_enabled = false;
                m_timer.cancel();
            }
            void cancel() {
                m_timer.cancel();
            }

            template<typename WaitHandler>
            void async_wait(boost::posix_time::seconds s,WaitHandler && handler) {
                std::unique_lock<std::mutex> _lock(m_mutex);
                auto time = is_enabled ? s : boost::posix_time::seconds(0);
                m_timer.expires_from_now(s);
                m_timer.async_wait(handler);
            }
    };

}