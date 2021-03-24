#include "network/p2p/PeerListener.h"
#include "common/utils.h"
#include <algorithm>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/system/error_code.hpp>
#include <deque>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <streambuf>
#include <string>

PeerListener::PeerListener(PeerId p
                          ,std::shared_ptr<Connection> conn
                          ,std::shared_ptr<Client> client) 
                          : m_peer(p)
                          , m_connection(conn)
                          , m_client(client) {
};

void PeerListener::parse_message(int length,MessageType mt,std::deque<char> &deq_buf) {
    switch (mt) {
        case MessageType::MT_Choke:
            cout << "<<< Choke" << endl;
            m_client->received_choke(m_peer);
            break;
        case MessageType::MT_UnChoke: 
            cout << "<<< Unchoke" << endl;
            m_client->received_unchoke(m_peer);
            break;
        case MessageType::MT_Interested: 
            cout << "<<< Interested" << endl;
            m_client->received_interested(m_peer);
            break;
        case MessageType::MT_NotInterested:
            cout << "<<< Not interested" << endl; 
            m_client->received_not_interested(m_peer);
            break;
        case MessageType::MT_Have: {
            auto h = Have::from_bytes_repr(deq_buf);
            cout << "<<< " + h->pprint() << endl;
            m_client->received_have(m_peer, h->m_piece_index);
            break;
        }
        case MessageType::MT_Bitfield: {
            auto bf = Bitfield::from_bytes_repr(length, deq_buf);
            cout << "<<< " + bf->pprint() << endl;
            m_client->received_bitfield(m_peer, *bf.get());
            break;
        }
        case MessageType::MT_Request: {
            auto r = Request::from_bytes_repr(deq_buf);
            cout << "<<< " + r->pprint() << endl;
            m_client->received_request(m_peer, *r.get());
            break;
        }
        case MessageType::MT_Piece: {
            auto p = Piece::from_bytes_repr(length, deq_buf);
            cout << "<<< " + p->pprint() << endl;
            m_client->received_piece(m_peer, *p.get());
            break;
        }
        case MessageType::MT_Cancel: {
            cout << "<<< Cancel" << endl;
            auto c = Cancel::from_bytes_repr(deq_buf);
            // m_client->received_cancel(m_peer, *c.get());
            break;
        }
        case MessageType::MT_Port: {
            cout << "<<< Port" << endl;
            // auto p = Port::from_bytes_repr(deq_buf);
            // m_client->receive
            break;
        }
        default:
            throw mt;
    }
}

void PeerListener::cancel_connection() {

}

Response PeerListener::receive_handshake() {

    std::promise<Response> promise;

    auto received_handshake = [&promise] (auto error,auto &&deq_buf) {
        if(error == 0) {
            auto msg = std::make_unique<HandShake>(HandShake::from_bytes_repr(deq_buf.size() - 48, deq_buf));
            promise.set_value(Response { {msg}, error });
        } else{
            promise.set_value(Response { {}, error });
        }
    };

    std::future<Response> future = promise.get_future();
    std::async(m_connection->read_message_timed(boost::bind(&received_handshake,_1,_2)));

    return future.get();
}

void PeerListener::read_messages() {
    m_connection->read_message(boost::bind(&PeerListener::read_message_body,shared_from_this(),_1));
}

void PeerListener::read_message_body(boost::system::error_code error, std::deque<char> &&deq_buf) {
    // This should be a KeepAlive message
    if(deq_buf.size() == 0) { 
        cout << "<<< KeepAlive" << endl;
        read_messages(); return; /* m_client->received */
    }

    // first character is always a message id
    unsigned char m_messageId = deq_buf.front();
    deq_buf.pop_front();

    // parse the payload
    parse_message(deq_buf.size(),messageType_from_id(m_messageId),deq_buf);

    // continue to read more messages
    read_messages();
}

PeerId PeerListener::get_peerId() {
    return m_peer;
}