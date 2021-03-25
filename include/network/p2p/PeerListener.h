#pragma once

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <streambuf>

#include "network/p2p/Connection.h"
#include "network/p2p/Message.h"
#include "network/p2p/Response.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/ConnectionEnded.h"
#include "network/p2p/Client.h"

using namespace boost::asio;
using ip::tcp;

class PeerListener : public enable_shared_from_this<PeerListener> {
    PeerId m_peer;

    std::shared_ptr<Connection> m_connection;
    std::shared_ptr<Client> m_client;

    private:
        void read_message_body(boost::system::error_code error, std::shared_ptr<std::deque<char>> deq_buf);

    public:
        PeerListener(PeerId m_peer
                    ,std::shared_ptr<Connection> conn
                    ,std::shared_ptr<Client> client);

        PeerId get_peerId();

        void parse_message(int length,MessageType mt,std::shared_ptr<std::deque<char>> deq_buf_ptr);
        void read_messages();

        void cancel_connection();

        Response receive_handshake();

        void stop();
};