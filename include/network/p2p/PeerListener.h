#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <streambuf>

#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/ConnectionEnded.h"
#include "network/p2p/Client.h"

using namespace boost::asio;
using ip::tcp;

class PeerListener {
    bool m_run = false;
    bool m_connected = false;
    std::shared_ptr<tcp::socket> m_socket;
    boost::asio::io_context::strand m_strand;
    std::shared_ptr<Client> m_client;
    PeerId m_peer;

    std::unique_ptr<boost::asio::streambuf> m_streambuf;

    public:
        PeerListener(PeerId m_peer
                    ,std::shared_ptr<Client> client
                    ,boost::asio::io_context::strand strand
                    ,std::shared_ptr<tcp::socket> socket);

        PeerId get_peerId();

        void read_message_length(boost::system::error_code error, size_t size);
        void read_message_body(boost::system::error_code error, size_t size, int length);

        std::unique_ptr<IMessage> wait_message();

        std::unique_ptr<HandShake> receive_handshake();

        void stop();
};