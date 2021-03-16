#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <streambuf>

#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/ConnectionEnded.h"
#include "network/p2p/Client.h"

using namespace boost::asio;
using ip::tcp;

class PeerListener : public enable_shared_from_this<PeerListener> {
    bool m_run = false;
    bool m_connected = false;
    std::shared_ptr<tcp::socket> m_socket;
    std::shared_ptr<Client> m_client;
    PeerId m_peer;

    std::unique_ptr<boost::asio::streambuf> m_streambuf;

    public:
        PeerListener(PeerId m_peer
                    ,std::shared_ptr<Client> client
                    ,std::shared_ptr<tcp::socket> socket);

        PeerId get_peerId();

        void parse_message(int length,MessageType mt,std::deque<char> &deq_buf);
        void read_messages();
        void read_message_length(boost::system::error_code error, size_t size);
        void read_message_body(boost::system::error_code error, size_t size, int length, int remaining);

        std::unique_ptr<HandShake> receive_handshake();

        void stop();
};