#pragma once

#include <iterator>
#include <memory>
#include <neither/maybe.hpp>
#include <string>
#include <vector>
#include <deque>

class IMessage {
    public:
        virtual std::vector<char> to_bytes_repr() const = 0;
};

class HandShake : public IMessage {
    char m_pstrlen;
    std::string m_pstr;
    char m_reserved[8];
    std::string m_url_info_hash;
    std::string m_peer_id;
    public:
        HandShake(unsigned char pstrlen,std::string pstr,char (&reserved)[8],std::string url_info_hash,std::string peer_id);
        std::vector<char> to_bytes_repr() const;
        static std::unique_ptr<HandShake> from_bytes_repr(unsigned char len,std::deque<char> &&bytes);
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
        static std::unique_ptr<Have> from_bytes_repr(std::deque<char> &bytes);
};

class Bitfield : public IMessage {
    int m_len;
    char m_messageId = 5;
    std::vector<bool> m_bitfield;
    public:
        Bitfield(int len,const std::vector<bool> &bitfield);
        std::vector<char> to_bytes_repr() const;
        static std::unique_ptr<Bitfield> from_bytes_repr(int len, std::deque<char> &bytes);
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
        static std::unique_ptr<Request> from_bytes_repr(std::deque<char> &bytes);
};

class Piece {
    int m_len;
    char m_messageId = 7;
    int m_index;
    int m_begin;
    std::vector<char> m_block;
    
    public:
        Piece(int index,int begin, std::vector<char> &&block);
        std::vector<char> to_bytes_repr() const;
        static std::unique_ptr<Piece> from_bytes_repr(int m_len,std::deque<char> &&bytes);
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
        static std::unique_ptr<Cancel> from_bytes_repr(std::deque<char> &bytes);
};

class Port {
    int m_len = 3;
    char m_messageId = 9;
    int m_port;

    public:
        Port(int port);
        std::vector<char> to_bytes_repr() const;
        static std::unique_ptr<Port> from_bytes_repr(std::deque<char> &bytes);
};
