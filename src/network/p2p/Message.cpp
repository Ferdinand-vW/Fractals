#include "network/p2p/Message.h"
#include "common/utils.h"
#include <memory>
#include <vector>


HandShake::HandShake(unsigned char pstrlen,std::string pstr,std::string url_info_hash,std::string peer_id) {
    m_pstrlen = pstrlen;
    m_pstr = pstr;
    for(int i = 0; i < 8; i++) { m_reserved[i] = 0; }
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

std::vector<char> Have::to_bytes_repr() const {
    std::vector<char> v = int_to_bytes(m_len);
    v.push_back(m_messageId);
    auto piece_bytes = int_to_bytes(m_piece_index);
    v.insert(v.end(),piece_bytes.begin(),piece_bytes.end());
    return v;
}

Bitfield::Bitfield(int len,std::vector<bool> bitfield) {
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

Piece::Piece(int index,int begin, std::unique_ptr<std::vector<char>> block) {
    m_len = 9 + block->size();
    m_index = index;
    m_begin = begin;
    m_block = std::move(block); //pass ownership to Piece message
}

std::vector<char> Piece::to_bytes_repr() const {
    std::vector<char> v(int_to_bytes(m_len));
    v.push_back(m_messageId);
    auto index_bytes = int_to_bytes(m_index);
    auto begin_bytes = int_to_bytes(m_begin);
    v.insert(v.end(),index_bytes.begin(),index_bytes.end());
    v.insert(v.end(),begin_bytes.begin(),begin_bytes.end());
    v.insert(v.end(),m_block->begin(),m_block->end());

    return v;
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