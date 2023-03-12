#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "fractals/common/utils.h"
#include "fractals/common/logger.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/Message.h"
#include "fractals/network/p2p/MessageType.h"

using namespace fractals::common;

namespace fractals::network::p2p {

    // HandShake::HandShake(unsigned char pstrlen,std::string pstr,char (&reserved)[8],std::vector<char> info_hash,std::vector<char> peer_id) {
    //     m_pstrlen = pstrlen;
    //     m_pstr = pstr;
    //     std::memcpy(m_reserved,reserved,sizeof(reserved));
    //     m_info_hash = info_hash;
    //     m_peer_id = peer_id;
    // }

    // std::optional<MessageType> HandShake::get_messageType() {
    //     return {};
    // }

    // int HandShake::get_length() {
    //     int pstrlen = static_cast<int>(static_cast<unsigned char>(m_pstrlen)) ;
    //     return pstrlen + strlen(m_reserved) + m_info_hash.size() + m_peer_id.size();
    // }

    // std::vector<char> HandShake::to_bytes_repr() const {
    //     std::vector<char> v;
    //     v.push_back(m_pstrlen);
    //     std::copy(m_pstr.begin(),m_pstr.end(),std::back_inserter(v));
    //     v.insert(v.end(),m_reserved,m_reserved+8);
    //     v.insert(v.end(),m_info_hash.begin(),m_info_hash.end());
    //     v.insert(v.end(),m_peer_id.begin(),m_peer_id.end());
    //     return v;
    // }

    // std::string HandShake::pprint() const {
    //     std::string base("HandShake");
    //     auto pstrlen_str = std::to_string(m_pstrlen);
    //     auto reserved_str = std::string(m_reserved);
        
    //     return base + " " + pstrlen_str + " " + reserved_str;
    // }

    // std::unique_ptr<HandShake> HandShake::from_bytes_repr(unsigned char len,std::deque<char> &bytes) {
    //     // -1 since pstrlen was already parsed
    //     if(len + 49 - 1 != bytes.size()) { 
    //         return {};
    //     }

    //     std::string pstr(bytes.begin(),bytes.begin() + len);
    //     char reserved[8];
    //     for(int i = 0; i < 8; i++) {
    //         reserved[i] = bytes[len + i];
    //     }
    //     std::vector<char> info_hash(bytes.begin() + len + 8,bytes.begin() + len + 28);
    //     std::vector<char> peer_id(bytes.begin() + len + 29,bytes.begin() + len + 49);

    //     return std::make_unique<HandShake>(HandShake(len,pstr,reserved,info_hash,peer_id));
    // }

    // std::vector<char> KeepAlive::to_bytes_repr() const {
    //     return int_to_bytes(m_len);
    // }

    // std::string KeepAlive::pprint() const {
    //     return "KeepAlive";
    // }

    // std::optional<MessageType> KeepAlive::get_messageType() {
    //     return {};
    // }

    // int KeepAlive::get_length() {
    //     return m_len;
    // }

    // std::vector<char> Choke::to_bytes_repr() const {
    //     std::vector<char> v = int_to_bytes(m_len);
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     return v;
    // }

    // std::string Choke::pprint() const {
    //     return "Choke";
    // }

    // std::optional<MessageType> Choke::get_messageType() {
    //     return m_messageType;
    // }

    // int Choke::get_length() {
    //     return m_len;
    // }

    // std::vector<char> UnChoke::to_bytes_repr() const {
    //     std::vector<char> v = int_to_bytes(m_len);
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     return v;
    // }

    // std::string UnChoke::pprint() const {
    //     return "UnChoke";
    // }

    // std::optional<MessageType> UnChoke::get_messageType() {
    //     return m_messageType;
    // }

    // int UnChoke::get_length() {
    //     return m_len;
    // }

    // std::vector<char> Interested::to_bytes_repr() const {
    //     std::vector<char> v = int_to_bytes(m_len);
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     return v;
    // }

    // std::string Interested::pprint() const {
    //     return "Interested";
    // }

    // std::optional<MessageType> Interested::get_messageType() {
    //     return m_messageType;
    // }

    // int Interested::get_length() {
    //     return m_len;
    // }

    // std::vector<char> NotInterested::to_bytes_repr() const {
    //     std::vector<char> v = int_to_bytes(m_len);
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     return v;
    // }

    // std::string NotInterested::pprint() const {
    //     return "NotInterested";
    // }

    // std::optional<MessageType> NotInterested::get_messageType() {
    //     return m_messageType;
    // }

    // int NotInterested::get_length() {
    //     return m_len;
    // }

    // Have::Have(int piece_index) {
    //     m_piece_index = piece_index;
    // }

    // std::optional<MessageType> Have::get_messageType() {
    //     return m_messageType;
    // }

    // int Have::get_length() {
    //     return m_len;
    // }

    // std::vector<char> Have::to_bytes_repr() const {
    //     std::vector<char> v = int_to_bytes(m_len);
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     auto piece_bytes = int_to_bytes(m_piece_index);
    //     v.insert(v.end(),piece_bytes.begin(),piece_bytes.end());
    //     return v;
    // }

    // std::string Have::pprint() const {
    //     return "Have " + std::to_string(m_piece_index);
    // }

    // std::unique_ptr<Have> Have::from_bytes_repr(std::deque<char> &bytes) {
    //     int m_piece_index = bytes_to_int(bytes);
    //     return std::make_unique<Have>(Have(m_piece_index));
    // }

    // Bitfield::Bitfield(const std::vector<bool> &bitfield) {
    //     m_bitfield = bitfield_to_bytes(bitfield);
    //     m_len = 1 + m_bitfield.size();
    // }

    // std::optional<MessageType> Bitfield::get_messageType() {
    //     return m_messageType;
    // }

    // int Bitfield::get_length() {
    //     return m_len;
    // }

    // std::vector<char> Bitfield::to_bytes_repr() const {
    //     std::vector<char> v = int_to_bytes(m_len);
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     v.insert(v.end(),m_bitfield.begin(),m_bitfield.end());
    //     return v;
    // }

    // std::string Bitfield::pprint() const {
    //     return "Bitfield " + std::to_string(m_bitfield.size());
    // }

    // std::unique_ptr<Bitfield> Bitfield::from_bytes_repr(int m_len,std::deque<char> &bytes) {
    //     auto m_bitfield = bytes_to_bitfield(m_len, bytes);

    //     return std::make_unique<Bitfield>(Bitfield(m_bitfield));
    // }

    // Request::Request(int index, int begin, int length) {
    //     m_index = index;
    //     m_begin = begin;
    //     m_length = length;
    // }

    // std::optional<MessageType> Request::get_messageType() {
    //     return m_messageType;
    // }

    // int Request::get_length() {
    //     return m_len;
    // }

    // std::vector<char> Request::to_bytes_repr() const {
    //     std::vector<char> v(int_to_bytes(m_len));
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     auto index_bytes = int_to_bytes(m_index);
    //     auto begin_bytes = int_to_bytes(m_begin);
    //     auto length_bytes = int_to_bytes(m_length);
    //     v.insert(v.end(),index_bytes.begin(),index_bytes.end());
    //     v.insert(v.end(),begin_bytes.begin(),begin_bytes.end());
    //     v.insert(v.end(),length_bytes.begin(),length_bytes.end());
    //     return v;
    // }

    // std::string Request::pprint() const {
    //     auto index_str = std::to_string(m_index);
    //     auto begin_str = std::to_string(m_begin);
    //     auto length_str = std::to_string(m_length);

    //     return "Request " + index_str + " " + begin_str + " " + length_str;
    // }

    // std::unique_ptr<Request> Request::from_bytes_repr(std::deque<char> &bytes) {
    //     int m_index  = bytes_to_int(bytes);
    //     int m_begin  = bytes_to_int(bytes);
    //     int m_length = bytes_to_int(bytes);

    //     return std::make_unique<Request>(Request(m_index,m_begin,m_length));
    // }

    // Piece::Piece(int index,int begin, std::vector<char> &&block) {
    //     m_len = 9 + block.size();
    //     m_index = index;
    //     m_begin = begin;
    //     m_block = block; //pass ownership to Piece message
    // }

    // std::optional<MessageType> Piece::get_messageType() {
    //     return m_messageType;
    // }

    // int Piece::get_length() {
    //     return m_len;
    // }

    // std::vector<char> Piece::to_bytes_repr() const {
    //     std::vector<char> v(int_to_bytes(m_len));
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     auto index_bytes = int_to_bytes(m_index);
    //     auto begin_bytes = int_to_bytes(m_begin);
    //     v.insert(v.end(),index_bytes.begin(),index_bytes.end());
    //     v.insert(v.end(),begin_bytes.begin(),begin_bytes.end());
    //     v.insert(v.end(),m_block.begin(),m_block.end());

    //     return v;
    // }

    // std::string Piece::pprint() const {
    //     auto index_str = std::to_string(m_index);
    //     auto begin_str = std::to_string(m_begin);
    //     auto length_str = std::to_string(m_block.size());
        
    //     return "Request " + index_str + " " + begin_str + " " + length_str;
    // }

    // std::unique_ptr<Piece> Piece::from_bytes_repr(int m_len,std::deque<char> &bytes) {
    //     int m_index = bytes_to_int(bytes);
    //     int m_begin = bytes_to_int(bytes);
    //     std::vector<char> m_block(bytes.begin(),bytes.begin() + m_len - 8); // 4 + 4
    //     return std::make_unique<Piece>(Piece(m_index,m_begin,std::move(m_block)));
    // }

    // Cancel::Cancel(int index, int begin, int length) {
    //     m_index = index;
    //     m_begin = begin;
    //     m_length = length;
    // }

    // std::optional<MessageType> Cancel::get_messageType() {
    //     return m_messageType;
    // }

    // int Cancel::get_length() {
    //     return m_len;
    // }

    // std::vector<char> Cancel::to_bytes_repr() const {
    //     std::vector<char> v(int_to_bytes(m_len));
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     auto index_bytes = int_to_bytes(m_index);
    //     auto begin_bytes = int_to_bytes(m_begin);
    //     auto length_bytes = int_to_bytes(m_length);
    //     v.insert(v.end(),index_bytes.begin(),index_bytes.end());
    //     v.insert(v.end(),begin_bytes.begin(),begin_bytes.end());
    //     v.insert(v.end(),length_bytes.begin(),length_bytes.end());

    //     return v;
    // }

    // std::string Cancel::pprint() const {
    //     return "Cancel";
    // }

    // std::unique_ptr<Cancel> Cancel::from_bytes_repr(std::deque<char> &bytes) {
    //     int m_index = bytes_to_int(bytes);
    //     int m_begin = bytes_to_int(bytes);
    //     int m_length = bytes_to_int(bytes);

    //     return std::make_unique<Cancel>(Cancel(m_index,m_begin,m_length));
    // }

    // Port::Port(int port) {
    //     m_port = port;
    // }

    // std::optional<MessageType> Port::get_messageType() {
    //     return m_messageType;
    // }

    // int Port::get_length() {
    //     return m_len;
    // }

    // std::vector<char> Port::to_bytes_repr() const {
    //     std::vector<char> v(int_to_bytes(m_len));
    //     auto m_id = static_cast<char>(messageType_to_id(m_messageType));
    //     v.push_back(m_id);
    //     auto port_bytes = int_to_bytes(m_port);
    //     v.insert(v.end(),port_bytes.begin(),port_bytes.end());

    //     return v;
    // }

    // std::string Port::pprint() const {
    //     return "Port";
    // }

    // std::unique_ptr<Port> Port::from_bytes_repr(std::deque<char> &bytes) {
    //     int m_port = bytes_to_int(bytes);

    //     return std::make_unique<Port>(Port(m_port));
    // }

    // std::unique_ptr<IMessage> IMessage::parse_message(http::PeerId p,int length,std::deque<char> &&deq_buf) {
    //     if(length == 0) {return std::make_unique<KeepAlive>(); }
        
    //     auto &lg = logger::get();
    //     length--;
    //     auto mt = messageType_from_id(deq_buf.front());
    //     deq_buf.pop_front();
    //     switch (mt) {
    //         case MessageType::MT_Choke:
    //             BOOST_LOG(lg) << "Choke <<< " << p.m_ip;
    //             return std::make_unique<Choke>();
    //             break;
    //         case MessageType::MT_UnChoke: 
    //             BOOST_LOG(lg) << "Unchoke <<< " << p.m_ip;
    //             return std::make_unique<UnChoke>();
    //             break;
    //         case MessageType::MT_Interested: 
    //             BOOST_LOG(lg) << "Interested <<< " << p.m_ip;
    //             return std::make_unique<Interested>();
    //             break;
    //         case MessageType::MT_NotInterested:
    //             BOOST_LOG(lg) << "Not interested <<< " << p.m_ip;
    //             return std::make_unique<NotInterested>();
    //             break;
    //         case MessageType::MT_Have: {
    //             auto h = Have::from_bytes_repr(deq_buf);
    //             BOOST_LOG(lg) << h->pprint() << " <<< " << p.m_ip;
    //             return h;
    //             break;
    //         }
    //         case MessageType::MT_Bitfield: {
    //             auto bf = Bitfield::from_bytes_repr(length, deq_buf);
    //             BOOST_LOG(lg) << bf->pprint() << " <<< " << p.m_ip;
    //             return bf;
    //             break;
    //         }
    //         case MessageType::MT_Request: {
    //             auto r = Request::from_bytes_repr(deq_buf);
    //             BOOST_LOG(lg) << r->pprint() << " <<< " << p.m_ip;
    //             return r;
    //             break;
    //         }
    //         case MessageType::MT_Piece: {
    //             auto pc = Piece::from_bytes_repr(length, deq_buf);
    //             BOOST_LOG(lg) << pc->pprint() << " <<< " << p.m_ip;
    //             return pc;
    //             break;
    //         }
    //         case MessageType::MT_Cancel: {
    //             BOOST_LOG(lg) << "Cancel <<< " << p.m_ip;
    //             auto c = Cancel::from_bytes_repr(deq_buf);
    //             return c;
    //             break;
    //         }
    //         case MessageType::MT_Port: {
    //             BOOST_LOG(lg) << "Port <<< " << p.m_ip;
    //             // auto p = Port::from_bytes_repr(deq_buf);
    //             // m_client->receive
    //             throw mt;
    //             break;
    //         }
    //         default:
    //             throw mt;
    //     }
    // }

}