#pragma once

#include <iterator>
#include <neither/maybe.hpp>
#include <string>
#include <vector>

// class Message {
//     private:
//         Message(int len) : length(len) {};
//         Message(int len,char mId) : length(len),messageId({mId}) {};
//         Message(int len,char mId,std::vector<char> pl) : length(len),messageId({mId}),payload(pl){};

//     public:
//         const int length;
//         const neither::Maybe<char> messageId;
//         const std::vector<char> payload;

//         static Message keepAlive() {
//             return Message(0);
//         }

//         static Message choke() {
//             return Message(1,0);
//         }

//         static Message unchoke() {
//             return Message(1,1);
//         }

//         static Message interested() {
//             return Message(1,2);
//         }

//         static Message not_interested() {
//             return Message(1,3);
//         }

//         static Message have(std::vector<char> piece_index) {
//             return Message(5,4,piece_index);
//         }

//         static Message bitfield(int len,std::vector<bool> bitfield) {
//             return Message(len,5,bitfield);
//         }
// };

class Message {
    public:
        virtual std::vector<char> byte_repr();
};

class HandShake : Message {
    char pstrlen;
    std::string pstr;
    char reserved[8];
    std::string url_info_hash;
    std::string peer_id;
    public:
        HandShake(unsigned char pstrlen_,std::string pstr_,std::string url_info_hash_,std::string peer_id_) {
            pstrlen = pstrlen_;
            pstr = pstr_;
            for(int i = 0; i < 8; i++) { reserved[i] = 0; }
            url_info_hash = url_info_hash_;
            peer_id = peer_id_;
        }

        std::vector<char> byte_repr() {
            std::vector<char> v;
            v.push_back(pstrlen);
            std::copy(pstr.begin(),pstr.end(),std::back_inserter(v));
            v.insert(v.end(),reserved,reserved+8);
            v.insert(v.end(),url_info_hash.begin(),url_info_hash.end());
            v.insert(v.end(),peer_id.begin(),peer_id.end());
            return v;
        }
    
};

struct KeepAlive {
    int length = 0;
};

struct Choke {
    int length = 1;
    char messageId = 0;
};

struct UnChoke {
    int length = 1;
    char messageId = 1;
};

struct Interested {
    int length = 1;
    char messageId = 2;
};

struct NotInterested {
    int length = 1;
    char messageId = 3;
};

struct Have {
    int length = 5;
    char messageId = 4;
    int piece_index;
};

class Bitfield {
    int length;
    char messageId = 5;
    std::vector<bool> bitfield;
    public:
        Bitfield(int len,std::vector<bool> bitfield_) {
            length = 1 + len;
            bitfield = bitfield_;
        } 
};

class Request {
    int len = 13;
    char messageId = 6;
    int index;
    int begin;
    int length;

    public:
        Request(int index_,int begin_,int length_) {
            index = index_;
            begin = begin_;
            length = length_;
        }
};

class Piece {
    int len;
    char messageId = 7;
    int index;
    int begin;
    std::vector<char> block;
    
    public:
        Piece(int index_,int begin_, std::vector<char> block_) {
            index = index_;
            begin = begin_;
            block = block_;
        }
};

class Cancel {
    int len = 13;
    char messageId = 8;
    int index;
    int begin;
    int length;

    public:
        Cancel(int index_,int begin_,int length_) {
            index = index_;
            begin = begin_;
            length = length_;
        }
};

class Port {
    int len = 3;
    char messageId = 9;
    ushort port;

    public:
        Port(int port_) : port(port_) {};
};
