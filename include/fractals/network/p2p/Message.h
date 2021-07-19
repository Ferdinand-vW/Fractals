#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "fractals/network/p2p/MessageType.h"

namespace fractals::network::http { class PeerId; }

namespace fractals::network::p2p {

    class IMessage {
        public:
            // virtual int get_length();
            virtual std::optional<MessageType> get_messageType() = 0;
            static std::unique_ptr<IMessage> parse_message (http::PeerId p,int length,std::deque<char> &&deq);
            // length specified by BitTorrent specification
            virtual int get_length() = 0;
            virtual std::vector<char> to_bytes_repr() const = 0;
            virtual std::string pprint() const = 0;
            virtual ~IMessage() {};
    };

    class HandShake : public IMessage {
        public:
            char m_pstrlen;
            std::string m_pstr;
            char m_reserved[8];
            std::vector<char> m_info_hash;
            std::vector<char> m_peer_id;

            HandShake(unsigned char pstrlen,std::string pstr,char (&reserved)[8],std::vector<char> info_hash,std::vector<char> peer_id);
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
            std::optional<MessageType> get_messageType();
            // Does not include size of char pstrlen
            int get_length();
            static std::unique_ptr<HandShake> from_bytes_repr(unsigned char len,std::deque<char> &bytes);
    };

    class KeepAlive : public IMessage {
        int m_len = 0;
        public:
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
    };

    class Choke : public IMessage {
        int m_len = 1;
        MessageType m_messageType = MessageType::MT_Choke;
        public:
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
    };

    class UnChoke : public IMessage {
        int m_len = 1;
        MessageType m_messageType = MessageType::MT_UnChoke;
        public:
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
    };

    class Interested : public IMessage {
        int m_len = 1;
        MessageType m_messageType = MessageType::MT_Interested;
        public:
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
    };

    class NotInterested : public IMessage {
        int m_len = 1;
        MessageType m_messageType = MessageType::MT_NotInterested;
        public:
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
    };

    class Have : public IMessage {
        int m_len = 5;
        MessageType m_messageType = MessageType::MT_Have;
        
        public:
            int m_piece_index;

            Have(int piece_index);
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
            static std::unique_ptr<Have> from_bytes_repr(std::deque<char> &bytes);
    };

    class Bitfield : public IMessage {
        int m_len;
        MessageType m_messageType = MessageType::MT_Bitfield;
        
        public:
            std::vector<char> m_bitfield;

            Bitfield(const std::vector<bool> &bitfield);
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            static std::unique_ptr<Bitfield> from_bytes_repr(int len, std::deque<char> &bytes);
            std::string pprint() const;
    };

    class Request : public IMessage {
        int m_len = 13;
        MessageType m_messageType = MessageType::MT_Request;
        
        public:
            int m_index;
            int m_begin;
            int m_length;

            Request(int index,int begin,int length);
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
            static std::unique_ptr<Request> from_bytes_repr(std::deque<char> &bytes);
    };

    class Piece : public IMessage {
        int m_len;
        MessageType m_messageType = MessageType::MT_Piece;
        
        public:
            int m_index;
            int m_begin;
            std::vector<char> m_block;

            Piece(int index,int begin, std::vector<char> &&block);
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
            static std::unique_ptr<Piece> from_bytes_repr(int m_len,std::deque<char> &bytes);
    };

    class Cancel : public IMessage {
        int m_len = 13;
        MessageType m_messageType = MessageType::MT_Cancel;
        int m_index;
        int m_begin;
        int m_length;

        public:
            Cancel(int index,int begin,int length);
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
            static std::unique_ptr<Cancel> from_bytes_repr(std::deque<char> &bytes);
    };

    class Port : public IMessage {
        int m_len = 3;
        MessageType m_messageType = MessageType::MT_Port;
        int m_port;

        public:
            Port(int port);
            std::optional<MessageType> get_messageType();
            int get_length();
            std::vector<char> to_bytes_repr() const;
            std::string pprint() const;
            static std::unique_ptr<Port> from_bytes_repr(std::deque<char> &bytes);
    };

}