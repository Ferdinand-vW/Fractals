#pragma once

#include <boost/asio.hpp>

#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"
#include "network/p2p/ConnectionEnded.h"

using namespace boost::asio;
using ip::tcp;

class PeerListener {
    bool m_run = false;
    bool m_connected = false;
    std::shared_ptr<tcp::socket> m_socket;
    PeerId m_peer;

    public:
        PeerListener(PeerId m_peer,std::shared_ptr<tcp::socket> socket);

        PeerId get_peerId();

        std::unique_ptr<IMessage> wait_message();

        void stop();
};