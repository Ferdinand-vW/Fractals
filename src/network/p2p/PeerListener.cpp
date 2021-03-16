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

PeerListener::PeerListener(PeerId p
                          ,std::shared_ptr<Client> client
                          ,std::shared_ptr<tcp::socket> sock) : m_client(client),m_socket(sock),m_peer(p),m_streambuf(std::make_unique<boost::asio::streambuf>()) {

};

void PeerListener::parse_message(int length,MessageType mt,std::deque<char> &deq_buf) {
    switch (mt) {
        case MessageType::MT_Choke:
            cout << "received choke" << endl;
            m_client->received_choke(m_peer);
            break;
        case MessageType::MT_UnChoke: 
            cout << "received unchoke" << endl;
            m_client->received_unchoke(m_peer);
            break;
        case MessageType::MT_Interested: 
            cout << "received interested" << endl;
            m_client->received_interested(m_peer);
            break;
        case MessageType::MT_NotInterested:
            cout << "received not interested" << endl; 
            m_client->received_not_interested(m_peer);
            break;
        case MessageType::MT_Have: {
            cout << "received have" << endl;
            auto h = Have::from_bytes_repr(deq_buf);
            m_client->received_have(m_peer, h->m_piece_index);
            break;
        }
        case MessageType::MT_Bitfield: {
            cout << "received bitfield" << endl;
            auto bf = Bitfield::from_bytes_repr(length, deq_buf);
            m_client->received_bitfield(m_peer, *bf.get());
            break;
        }
        case MessageType::MT_Request: {
            cout << "received request" << endl;
            auto r = Request::from_bytes_repr(deq_buf);
            m_client->received_request(m_peer, *r.get());
            break;
        }
        case MessageType::MT_Piece: {
            cout << "received piece" << endl;
            auto p = Piece::from_bytes_repr(length, deq_buf);
            m_client->received_piece(m_peer, *p.get());
            break;
        }
        case MessageType::MT_Cancel: {
            cout << "received cancel" << endl;
            auto c = Cancel::from_bytes_repr(deq_buf);
            // m_client->received_cancel(m_peer, *c.get());
            break;
        }
        case MessageType::MT_Port: {
            cout << "received port" << endl;
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
    cout << "read messages" << endl;
    cout << m_streambuf->size() << endl;
    boost::asio::async_read(*m_socket.get()
                           ,*m_streambuf.get()
                           ,boost::asio::transfer_at_least(4)
                           ,boost::bind(&PeerListener::read_message_length,shared_from_this()
                           ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}

void PeerListener::read_message_length(boost::system::error_code error, size_t size) {
    cout << "read message length" << endl;
    cout << "error: " << error.message() << endl;
    std::deque<char> deq_buf;
    std::copy(boost::asio::buffers_begin(m_streambuf->data())
             ,boost::asio::buffers_end(m_streambuf->data())
             ,std::back_inserter(deq_buf));

    std::string length_hex = bytes_to_hex(deq_buf);
    std::cout << "length hex: " << length_hex << std::endl;
    std::cout << deq_buf.size() << endl;
    int m_length = bytes_to_int(deq_buf);
    std::cout << "message size: " << m_length << std::endl;
    std::cout << "m_streambuf: " << m_streambuf->size() << endl;
    std::cout << "deq buf size: " << deq_buf.size() << std::endl;
    std::cout << "read size: " << size << std::endl;
    std::cout << m_streambuf->size() << std::endl;
    m_streambuf->consume(4);

    boost::asio::async_read(*m_socket.get()
                           ,*m_streambuf.get()
                           ,boost::asio::transfer_at_least(m_length - size + 4)
                           ,boost::bind(&PeerListener::read_message_body,shared_from_this()
                           ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                           ,m_length
                           ,m_length - size + 4));
}


void PeerListener::read_message_body(boost::system::error_code error, size_t size, int length, int remaining) {
    cout << "read message body" << endl;
    cout << "error: " << error.message() << endl;
    if (size < remaining) {
        boost::asio::async_read(*m_socket.get()
                           ,*m_streambuf.get()
                           ,boost::asio::transfer_exactly(remaining - size)
                           ,boost::bind(&PeerListener::read_message_body,shared_from_this()
                           ,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred
                           ,length
                           ,remaining - size));

    } else {

        if(length == 0) { read_messages(); return; /* m_client->received */}

        std::deque<char> buff;
        std::copy(boost::asio::buffers_begin(m_streambuf->data())
                 ,boost::asio::buffers_end(m_streambuf->data())
                 ,back_inserter(buff));
        std::string hex_msg = bytes_to_hex(buff);
        std::cout << "hex_msg: " << hex_msg << std::endl;

        unsigned char m_messageId = buff.front();
        std::cout << (int)m_messageId << std::endl;
        std::cout << messageType_to_string(messageType_from_id(m_messageId)) << std::endl;
        buff.pop_front();

        parse_message(length,messageType_from_id(m_messageId),buff);

        m_streambuf->consume(length);

        read_messages();
    }
}

PeerId PeerListener::get_peerId() {
    return m_peer;
}