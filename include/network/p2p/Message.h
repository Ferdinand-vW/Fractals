#pragma once

#include <iterator>
#include <memory>
#include <neither/maybe.hpp>
#include <string>
#include <vector>

class IMessage {
    public:
        virtual std::vector<char> to_bytes_repr() const = 0;
        static std::unique_ptr<IMessage> from_bytes_repr(std::vector<char> bytes);
};

class HandShake : public IMessage {
    char m_pstrlen;
    std::string m_pstr;
    char m_reserved[8];
    std::string m_url_info_hash;
    std::string m_peer_id;
    public:
        HandShake(unsigned char pstrlen,std::string pstr,std::string url_info_hash,std::string peer_id);
        std::vector<char> to_bytes_repr() const;
};

class KeepAlive : public IMessage {
    int m_len = 0;
    public:
        std::vector<char> to_bytes_repr() const;
};

class Choke : public IMessage {
    int m_len = 1;
    char m_messageId = 0;
    public:
        std::vector<char> to_bytes_repr() const;
};

class UnChoke : public IMessage {
    int m_len = 1;
    char m_messageId = 1;
    public:
        std::vector<char> to_bytes_repr() const;
};

class Interested : public IMessage {
    int m_len = 1;
    char m_messageId = 2;
    public:
        std::vector<char> to_bytes_repr() const;
};

class NotInterested : public IMessage {
    int m_len = 1;
    char m_messageId = 3;
    public:
        std::vector<char> to_bytes_repr() const;
};

class Have : public IMessage {
    int m_len = 5;
    char m_messageId = 4;
    int m_piece_index;
    public:
        Have(int piece_index);
        std::vector<char> to_bytes_repr() const;
};

class Bitfield : public IMessage {
    int m_len;
    char m_messageId = 5;
    std::vector<bool> m_bitfield;
    public:
        Bitfield(int len,std::vector<bool> bitfield);
        std::vector<char> to_bytes_repr() const;
};

class Request : public IMessage {
    int m_len = 13;
    char m_messageId = 6;
    int m_index;
    int m_begin;
    int m_length;

    public:
        Request(int index,int begin,int length);
        std::vector<char> to_bytes_repr() const;
};

class Piece {
    int m_len;
    char m_messageId = 7;
    int m_index;
    int m_begin;
    std::unique_ptr<std::vector<char>> m_block;
    
    public:
        Piece(int index,int begin, std::unique_ptr<std::vector<char>> block);
        std::vector<char> to_bytes_repr() const;
};

class Cancel {
    int m_len = 13;
    char m_messageId = 8;
    int m_index;
    int m_begin;
    int m_length;

    public:
        Cancel(int index,int begin,int length);
        std::vector<char> to_bytes_repr() const;
};

class Port {
    int m_len = 3;
    char m_messageId = 9;
    int m_port;

    public:
        Port(int port);
        std::vector<char> to_bytes_repr() const;
};
