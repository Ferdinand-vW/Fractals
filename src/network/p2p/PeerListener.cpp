#include "network/p2p/PeerListener.h"
#include "common/utils.h"
#include "network/p2p/MessageType.h"
#include <algorithm>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>
#include <deque>
#include <iterator>
#include <mutex>
#include <streambuf>
#include <string>

PeerListener::PeerListener(PeerId p
                          ,std::shared_ptr<Client> client
                          ,std::shared_ptr<tcp::socket> sock) : m_client(client),m_socket(sock),m_peer(p),m_streambuf(std::make_unique<boost::asio::streambuf>()) {

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
            cout << "<<< Have " << h->m_piece_index << endl;
            m_client->received_have(m_peer, h->m_piece_index);
            break;
        }
        case MessageType::MT_Bitfield: {
            auto bf = Bitfield::from_bytes_repr(length, deq_buf);
            cout << "<<< Bitfield " << bf->m_bitfield.size() << endl;
            m_client->received_bitfield(m_peer, *bf.get());
            break;
        }
        case MessageType::MT_Request: {
            auto r = Request::from_bytes_repr(deq_buf);
            cout << "<<< Request " + std::to_string(r->m_index) + " " + std::to_string(r->m_begin) + " " + std::to_string(r->m_length) << endl;
            m_client->received_request(m_peer, *r.get());
            break;
        }
        case MessageType::MT_Piece: {
            auto p = Piece::from_bytes_repr(length, deq_buf);
            cout << "<<< Piece " + std::to_string(p->m_index) + " " + std::to_string(p->m_begin) + " " + std::to_string(p->m_block.size()) << endl;
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

std::unique_ptr<HandShake> PeerListener::receive_handshake() {
    boost::asio::streambuf buf;
    boost::system::error_code error;
    boost::asio::read(*m_socket.get(),buf,boost::asio::transfer_exactly(1),error);
    
    std::deque<char> deq_buf(boost::asio::buffers_begin(buf.data())
                            ,boost::asio::buffers_end(buf.data()));
    char pstrlen = deq_buf.front();
    deq_buf.pop_front();
    buf.consume(1);

    // 8 reserved bytes, 20 info hash , 20 peer id
    int n = (unsigned char)pstrlen + 48;

    while(n > 0) {
        int bytes_read = boost::asio::read(*m_socket.get(),buf,boost::asio::transfer_exactly(n),error);
        std::copy(boost::asio::buffers_begin(buf.data())
                 ,boost::asio::buffers_end(buf.data())
                 ,back_inserter(deq_buf));
        n -= bytes_read;
    }

    return HandShake::from_bytes_repr(pstrlen, deq_buf);
}

void PeerListener::read_messages() {
    cout << m_streambuf->size() << endl;
    boost::asio::async_read(*m_socket.get()
                           ,*m_streambuf.get()
                           ,boost::asio::transfer_exactly(4)
                           ,boost::bind(&PeerListener::read_message_length,shared_from_this()
                           ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}

void PeerListener::read_message_length(boost::system::error_code error, size_t size) {
    cout << "error: " << error.message() << endl;
    cout << "size: " << size << endl; // should equal exactly 4

    // copy data over into deque
    std::deque<char> deq_buf;
    std::copy(boost::asio::buffers_begin(m_streambuf->data())
             ,boost::asio::buffers_end(m_streambuf->data())
             ,std::back_inserter(deq_buf));

    // read length of exactly 4 bytes
    int m_length = bytes_to_int(deq_buf);
    m_streambuf->consume(4);

    // continue to read body using derived length
    boost::asio::async_read(*m_socket.get()
                           ,*m_streambuf.get()
                           ,boost::asio::transfer_exactly(m_length)
                           ,boost::bind(&PeerListener::read_message_body,shared_from_this()
                           ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                           ,m_length
                           ,m_length));
}


void PeerListener::read_message_body(boost::system::error_code error, size_t size, int length, int remaining) {
    cout << "error: " << error.message() << endl;
    cout << "size: " << size << endl;

    //if we haven't read everything requested then read some more
    if (size < remaining) {
        boost::asio::async_read(*m_socket.get()
                           ,*m_streambuf.get()
                           ,boost::asio::transfer_exactly(remaining - size)
                           ,boost::bind(&PeerListener::read_message_body,shared_from_this()
                           ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                           ,length
                           ,remaining - size)); // compute how much more we have to read

    } else {

        // This should be a KeepAlive message
        if(length == 0) { read_messages(); return; /* m_client->received */}

        std::deque<char> buff;
        std::copy(boost::asio::buffers_begin(m_streambuf->data())
                 ,boost::asio::buffers_end(m_streambuf->data())
                 ,back_inserter(buff));

        // first character is always a message id
        unsigned char m_messageId = buff.front();
        buff.pop_front();

        // parse the payload
        parse_message(length,messageType_from_id(m_messageId),buff);

        m_streambuf->consume(length);

        // continue to read more messages
        read_messages();
    }
}

PeerId PeerListener::get_peerId() {
    return m_peer;
}