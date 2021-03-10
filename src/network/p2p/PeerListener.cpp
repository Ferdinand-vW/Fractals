#include "network/p2p/PeerListener.h"
#include "common/utils.h"
#include <algorithm>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/system/error_code.hpp>
#include <deque>

PeerListener::PeerListener(PeerId p,std::shared_ptr<tcp::socket> sock) : m_socket(sock),m_peer(p) {};

std::unique_ptr<IMessage> parse_message(int m_length,int m_messageId,std::deque<char> &deq_buf) {
    switch (m_messageId) {
        case 0: return std::make_unique<Choke>(Choke());
        case 1: return std::make_unique<UnChoke>(UnChoke());
        case 2: return std::make_unique<Interested>(Interested());
        case 3: return std::make_unique<NotInterested>(NotInterested());
        case 4: return Have::from_bytes_repr(deq_buf);
        case 5: return Bitfield::from_bytes_repr(m_length, deq_buf);
        case 6: return Request::from_bytes_repr(deq_buf);
        case 7: return Piece::from_bytes_repr(m_length, deq_buf);
        case 8: return Cancel::from_bytes_repr(deq_buf);
        case 9: return Port::from_bytes_repr(deq_buf);
        default:
            throw m_messageId;
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

std::unique_ptr<IMessage> PeerListener::wait_message() {
    boost::asio::streambuf buf;
    boost::system::error_code error;
    int n = 4;
    // read the message length
    int n_bytes = boost::asio::read(*m_socket.get(),buf,boost::asio::transfer_exactly(n),error);

    std::deque<char> deq_buf(boost::asio::buffers_begin(buf.data()),boost::asio::buffers_end(buf.data()));
    int m_length = bytes_to_int(deq_buf);
    buf.consume(n_bytes);

    if (m_length == 0) { return std::make_unique<KeepAlive>(KeepAlive()); }

    n = m_length;
    while (n > 0) {
        n_bytes = boost::asio::read(*m_socket.get(),buf,boost::asio::transfer_exactly(m_length),error);
        n -= n_bytes;

        std::copy(boost::asio::buffers_begin(buf.data()),boost::asio::buffers_end(buf.data()),back_inserter(deq_buf));
    }

    char m_messageId = deq_buf.front();
    deq_buf.pop_front();

    return parse_message(m_length,(unsigned char)m_messageId,deq_buf);
        
}

PeerId PeerListener::get_peerId() {
    return m_peer;
}