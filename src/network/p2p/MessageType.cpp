#include "network/p2p/MessageType.h"

MessageType messageType_from_id(unsigned char m) {
    return static_cast<MessageType>(m);
}

unsigned char messageType_to_id(MessageType mt) {
    switch(mt) {
        case MessageType::MT_Choke:         return 0;
        case MessageType::MT_UnChoke:       return 1;
        case MessageType::MT_Interested:    return 2;
        case MessageType::MT_NotInterested: return 3;
        case MessageType::MT_Have:          return 4;
        case MessageType::MT_Bitfield:      return 5;
        case MessageType::MT_Request:       return 6;
        case MessageType::MT_Piece:         return 7;
        case MessageType::MT_Cancel:        return 8;
        case MessageType::MT_Port:          return 9;
    }
}

std::string messageType_to_string(MessageType mt) {
    switch(mt) {
        case MessageType::MT_Choke:         return std::string("choke");
        case MessageType::MT_UnChoke:       return std::string("unchoke");
        case MessageType::MT_Interested:    return std::string("interested");
        case MessageType::MT_NotInterested: return std::string("not interested");
        case MessageType::MT_Have:          return std::string("have");
        case MessageType::MT_Bitfield:      return std::string("bitfield");
        case MessageType::MT_Request:       return std::string("request");
        case MessageType::MT_Piece:         return std::string("piece");
        case MessageType::MT_Cancel:        return std::string("cancel");
        case MessageType::MT_Port:          return std::string("port");
    }
}