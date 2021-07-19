#pragma once

#include <string>

namespace fractals::network::p2p {

enum class MessageType { MT_Choke, MT_UnChoke, MT_Interested, MT_NotInterested, MT_Have
                       , MT_Bitfield, MT_Request, MT_Piece, MT_Cancel, MT_Port };

MessageType messageType_from_id(unsigned char m);
unsigned char messageType_to_id(MessageType mt);
std::string messageType_to_string(MessageType mt);

}