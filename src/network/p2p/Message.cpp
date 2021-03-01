#include "network/p2p/Message.h"
#include "common/utils.h"
#include <cstring>
#include <memory>
#include <vector>


HandShake::HandShake(unsigned char pstrlen,std::string pstr,char (&reserved)[8],std::string url_info_hash,std::string peer_id) {
    m_pstrlen = pstrlen;
    m_pstr = pstr;
    std::memcpy(m_reserved,reserved,sizeof(reserved));
    m_url_info_hash = url_info_hash;
    m_peer_id = peer_id;
}

std::vector<char> HandShake::to_bytes_repr() const {
    std::vector<char> v;
    v.push_back(m_pstrlen);
    std::copy(m_pstr.begin(),m_pstr.end(),std::back_inserter(v));
    v.insert(v.end(),m_reserved,m_reserved+8);
    v.insert(v.end(),m_url_info_hash.begin(),m_url_info_hash.end());
    v.insert(v.end(),m_peer_id.begin(),m_peer_id.end());
    return v;
}
std::unique_ptr<HandShake> HandShake::from_bytes_repr(unsigned char len,std::deque<char> &&bytes) {
    // -1 since pstrlen was already parsed
    if(len + 49 - 1 != bytes.size()) { 
        return {};
    }

    std::string pstr(bytes.begin(),bytes.begin() + len);
    char reserved[8];
    for(int i = 0; i < 8; i++) {
        reserved[i] = bytes[len + i];
    }
    std::string url_info_hash(bytes.begin() + len + 8,bytes.begin() + len + 28);
    std::string peer_id(bytes.begin() + len + 29,bytes.begin() + len + 49);

    return std::make_unique<HandShake>(HandShake(len,pstr,reserved,url_info_hash,peer_id));
}

std::vector<char> KeepAlive::to_bytes_repr() const {
    return int_to_bytes(m_len);
}

std::vector<char> Choke::to_bytes_repr() const {
    std::vector<char> v = int_to_bytes(m_len);
    v.push_back(m_messageId);
    return v;
}

std::vector<char> UnChoke::to_bytes_repr() const {
    std::vector<char> v = int_to_bytes(m_len);
    v.push_back(m_messageId);
    return v;
}

std::vector<char> Interested::to_bytes_repr() const {
    std::vector<char> v = int_to_bytes(m_len);
    v.push_back(m_messageId);
    return v;
}

std::vector<char> NotInterested::to_bytes_repr() const {
    std::vector<char> v = int_to_bytes(m_len);
    v.push_back(m_messageId);
    return v;
}

Have::Have(int piece_index) {
    m_piece_index = piece_index;
}

std::vector<char> Have::to_bytes_repr() const {
    std::vector<char> v = int_to_bytes(m_len);
    v.push_back(m_messageId);
    auto piece_bytes = int_to_bytes(m_piece_index);
    v.insert(v.end(),piece_bytes.begin(),piece_bytes.end());
    return v;
}

std::unique_ptr<Have> Have::from_bytes_repr(std::deque<char> &bytes) {
    int m_piece_index = bytes_to_int(bytes);
    return std::make_unique<Have>(Have(m_piece_index));
}

Bitfield::Bitfield(int len,const std::vector<bool> &bitfield) {
    m_len = 1 + len;
    m_bitfield = bitfield;
}

std::vector<char> Bitfield::to_bytes_repr() const {
    std::vector<char> v = int_to_bytes(m_len);
    v.push_back(m_messageId);
    std::vector<char> bitfield_bytes(bitfield_to_bytes(m_bitfield));
    v.insert(v.end(),bitfield_bytes.begin(),bitfield_bytes.end());
    return v;
}

std::unique_ptr<Bitfield> Bitfield::from_bytes_repr(int m_len,std::deque<char> &bytes) {
    auto m_bitfield = bytes_to_bitfield(m_len - 1, bytes);

    return std::make_unique<Bitfield>(Bitfield(m_len,m_bitfield));
}

Request::Request(int index, int begin, int length) {
    m_index = index;
    m_begin = begin;
    m_length = length;
}

std::vector<char> Request::to_bytes_repr() const {
    std::vector<char> v(int_to_bytes(m_len));
    v.push_back(m_messageId);
    auto index_bytes = int_to_bytes(m_index);
    auto begin_bytes = int_to_bytes(m_begin);
    auto length_bytes = int_to_bytes(m_length);
    v.insert(v.end(),index_bytes.begin(),index_bytes.end());
    v.insert(v.end(),begin_bytes.begin(),begin_bytes.end());
    v.insert(v.end(),length_bytes.begin(),length_bytes.end());
    return v;
}

std::unique_ptr<Request> Request::from_bytes_repr(std::deque<char> &bytes) {
    int m_index  = bytes_to_int(bytes);
    int m_begin  = bytes_to_int(bytes);
    int m_length = bytes_to_int(bytes);

    return std::make_unique<Request>(Request(m_index,m_begin,m_length));
}

Piece::Piece(int index,int begin, std::vector<char> &&block) {
    m_len = 9 + block.size();
    m_index = index;
    m_begin = begin;
    m_block = block; //pass ownership to Piece message
}

std::vector<char> Piece::to_bytes_repr() const {
    std::vector<char> v(int_to_bytes(m_len));
    v.push_back(m_messageId);
    auto index_bytes = int_to_bytes(m_index);
    auto begin_bytes = int_to_bytes(m_begin);
    v.insert(v.end(),index_bytes.begin(),index_bytes.end());
    v.insert(v.end(),begin_bytes.begin(),begin_bytes.end());
    v.insert(v.end(),m_block.begin(),m_block.end());

    return v;
}

std::unique_ptr<Piece> Piece::from_bytes_repr(int m_len,std::deque<char> &&bytes) {
    int m_index = bytes_to_int(bytes);
    int m_begin = bytes_to_int(bytes);
    std::vector<char> m_block(bytes.begin(),bytes.begin() + m_len - 9); // 1 + 4 + 4
    return std::make_unique<Piece>(Piece(m_index,m_begin,std::move(m_block)));
}

Cancel::Cancel(int index, int begin, int length) {
    m_index = index;
    m_begin = begin;
    m_length = length;
}

std::vector<char> Cancel::to_bytes_repr() const {
    std::vector<char> v(int_to_bytes(m_len));
    v.push_back(m_messageId);
    auto index_bytes = int_to_bytes(m_index);
    auto begin_bytes = int_to_bytes(m_begin);
    auto length_bytes = int_to_bytes(m_length);
    v.insert(v.end(),index_bytes.begin(),index_bytes.end());
    v.insert(v.end(),begin_bytes.begin(),begin_bytes.end());
    v.insert(v.end(),length_bytes.begin(),length_bytes.end());

    return v;
}

std::unique_ptr<Cancel> Cancel::from_bytes_repr(std::deque<char> &bytes) {
    int m_index = bytes_to_int(bytes);
    int m_begin = bytes_to_int(bytes);
    int m_length = bytes_to_int(bytes);

    return std::make_unique<Cancel>(Cancel(m_index,m_begin,m_length));
}

Port::Port(int port) {
    m_port = port;
}

std::vector<char> Port::to_bytes_repr() const {
    std::vector<char> v(int_to_bytes(m_len));
    v.push_back(m_messageId);
    auto port_bytes = int_to_bytes(m_port);
    v.insert(v.end(),port_bytes.begin(),port_bytes.end());

    return v;
}

std::unique_ptr<Port> Port::from_bytes_repr(std::deque<char> &bytes) {
    int m_port = bytes_to_int(bytes);

    return std::make_unique<Port>(Port(m_port));
}