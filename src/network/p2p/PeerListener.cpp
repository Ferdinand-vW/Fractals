#include "network/p2p/PeerListener.h"
#include "common/utils.h"
#include "network/p2p/MessageType.h"
#include <algorithm>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>
#include <deque>
#include <iterator>

PeerListener::PeerListener(PeerId p
                          ,std::shared_ptr<Client> client
                          ,std::shared_ptr<tcp::socket> sock) : m_client(client),m_socket(sock),m_peer(p) {

};

std::unique_ptr<IMessage> parse_message(int length,MessageType mt,std::deque<char> &deq_buf) {
    switch (mt) {
        case MessageType::MT_Choke: return std::make_unique<Choke>(Choke());
        case MessageType::MT_UnChoke: return std::make_unique<UnChoke>(UnChoke());
        case MessageType::MT_Interested: return std::make_unique<Interested>(Interested());
        case MessageType::MT_NotInterested: return std::make_unique<NotInterested>(NotInterested());
        case MessageType::MT_Have: return Have::from_bytes_repr(deq_buf);
        case MessageType::MT_Bitfield: return Bitfield::from_bytes_repr(length, deq_buf);
        case MessageType::MT_Request: return Request::from_bytes_repr(deq_buf);
        case MessageType::MT_Piece: return Piece::from_bytes_repr(length, deq_buf);
        case MessageType::MT_Cancel: return Cancel::from_bytes_repr(deq_buf);
        case MessageType::MT_Port: return Port::from_bytes_repr(deq_buf);
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


void PeerListener::read_message_length(boost::system::error_code error, size_t size) {

}


void PeerListener::read_message_body(boost::system::error_code error, size_t size, int length) {

}

std::unique_ptr<IMessage> PeerListener::wait_message() {
    boost::asio::streambuf buf;
    boost::system::error_code error;
    int n = 4;
    // read the message length
    int n_bytes = boost::asio::read(*m_socket.get(),buf,boost::asio::transfer_exactly(n),error);
    std::cout << "read messages " << n_bytes << std::endl;
    std::deque<char> deq_buf(boost::asio::buffers_begin(buf.data()),boost::asio::buffers_end(buf.data()));
    std::string length_hex = bytes_to_hex(deq_buf);
    std::cout << "length hex: " << length_hex << std::endl;
    int m_length = bytes_to_int(deq_buf);
    std::cout << "message size: " << m_length << std::endl;
    std::cout << "deq buf size: " << deq_buf.size() << std::endl;
    buf.consume(n_bytes);

    if (m_length == 0) { return std::make_unique<KeepAlive>(KeepAlive()); }

    n = m_length;
    while (n > 0) {
        n_bytes = boost::asio::read(*m_socket.get(),buf,boost::asio::transfer_exactly(n),error);
        n -= n_bytes;

        std::vector<char> buff;
        std::string msg(boost::asio::buffers_begin(buf.data())
                      ,boost::asio::buffers_end(buf.data()));
        std::copy(msg.begin(),msg.end(),back_inserter(buff));
        std::string hex_msg = bytes_to_hex(buff);
        std::cout << "hex_msg: " << hex_msg << std::endl;

        std::copy(boost::asio::buffers_begin(buf.data()),boost::asio::buffers_end(buf.data()),back_inserter(deq_buf));
        buf.consume(n_bytes);
    }

    std::string message_hex = bytes_to_hex(deq_buf);
    std::cout << "message hex: " << message_hex << std::endl;

    unsigned char m_messageId = deq_buf.front();
    std::cout << (int)m_messageId << std::endl;
    std::cout << messageType_to_string(messageType_from_id(m_messageId)) << std::endl;
    deq_buf.pop_front();

    return parse_message(m_length,messageType_from_id(m_messageId),deq_buf);
        
}



PeerId PeerListener::get_peerId() {
    return m_peer;
}