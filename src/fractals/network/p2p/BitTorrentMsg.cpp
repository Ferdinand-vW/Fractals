#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/common/utils.h"
#include <type_traits>
#include <utility>

namespace fractals::network::p2p
{
    std::ostream& operator<<(std::ostream& os, const BitTorrentMessage& msg)
    {
        std::visit(common::overloaded{
                [&](const auto& m) {
                    os << m;
                }
            }, msg);

        return os;
    }

    bool operator==(const BitTorrentMessage& lhs, const BitTorrentMessage& rhs)
    {
        return lhs.index() == rhs.index() &&
            std::visit(common::overloaded{
                [&](const auto& msg) {
                    using T = std::decay_t<decltype(msg)>;
                    return msg == std::get<T>(rhs);
                }
            }, lhs);
    }

    HandShake::HandShake(const std::string& pstr, std::array<char, 8>&& reserved,
                     std::array<char,20>&& infoHash, std::array<char, 20>&& peerId)
                     : mPstr(pstr), mReserved(std::move(reserved))
                     , mInfoHash(std::move(infoHash)), mPeerId(std::move(peerId)) {}

    uint32_t HandShake::getLen() const
    {
        return MSG_MIN_LEN + getPstrLn();
    }

    std::vector<char> HandShake::getPrefix() const
    {
        return {getPstrLn()};
    }

    char HandShake::getPstrLn() const
    {
        return mPstr.size();
    }

    const std::string& HandShake::getPstr() const
    {
        return mPstr;
    }

    const std::array<char, 8>& HandShake::getReserved() const
    {
        return mReserved;
    }

    const std::array<char, 20>& HandShake::getInfoHash() const
    {
        return mInfoHash;
    }

    const std::array<char, 20>& HandShake::getPeerId() const
    {
        return mPeerId;
    }

    bool operator==(const HandShake& h1, const HandShake& h2)
    {
        return h1.getPstrLn() == h2.getPstrLn() && h1.getPstr() == h2.getPstr()
            && h1.getReserved() == h2.getReserved() && h1.getInfoHash() == h2.getInfoHash()
            && h1.getPeerId() == h2.getPeerId();
    }

    std::ostream& operator<<(std::ostream& os, const HandShake& msg)
    {
        os << "HandShake{";
        os << "PstrLen: " << msg.getPstrLn() << ", ";
        os << "Pstr: " << msg.getPstr() << ", ";

        const auto reservedBytes = 
            common::bytes_to_hex(std::vector<char>(msg.getReserved().begin(), msg.getReserved().end()));
        os << "Reserved: " << reservedBytes << ", ";
        
        const auto ihBytes = 
            common::bytes_to_hex(std::vector<char>(msg.getInfoHash().begin(), msg.getInfoHash().end()));
        os << "InfoHash: " << ihBytes << ", ";

        const auto pBytes = 
            common::bytes_to_hex(std::vector<char>(msg.getPeerId().begin(), msg.getPeerId().end()));
        os << "PeerId: " << pBytes << "}";

        return os;
    }

    std::vector<char> KeepAlive::getPrefix() const
    {
        return common::int_to_bytes(MSG_LEN);
    }

    bool operator==(const KeepAlive&, const KeepAlive&)
    {
        return true;
    }

    std::ostream& operator<<(std::ostream& os, const KeepAlive& msg)
    {
        return os << "KeepAlive{}";
    }

    uint32_t KeepAlive::getLen() const
    {
        return MSG_LEN;
    }

    std::vector<char> Choke::getPrefix() const
    {   
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    bool operator==(const Choke&, const Choke&)
    {
        return true;
    }

    std::ostream& operator<<(std::ostream& os, const Choke& msg)
    {
        return os << "Choke{}";
    }

    uint32_t Choke::getLen() const
    {
        return MSG_LEN;
    }

    std::vector<char> UnChoke::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    bool operator==(const UnChoke&, const UnChoke&)
    {
        return true;
    }

    std::ostream& operator<<(std::ostream& os, const UnChoke& msg)
    {
        return os << "UnChoke{}";
    }

    uint32_t UnChoke::getLen() const
    {
        return MSG_LEN;
    }

    std::vector<char> Interested::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    bool operator==(const Interested&, const Interested&)
    {
        return true;
    }

    std::ostream& operator<<(std::ostream& os, const Interested& msg)
    {
        return os << "Interested{}";
    }

    uint32_t Interested::getLen() const
    {
        return MSG_LEN;
    }

    std::vector<char> NotInterested::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    bool operator==(const NotInterested&, const NotInterested&)
    {
        return true;
    }

    std::ostream& operator<<(std::ostream& os, const NotInterested& msg)
    {
        return os << "NotInterested{}";
    }

    uint32_t NotInterested::getLen() const
    {
        return MSG_LEN;
    }

    Have::Have(uint32_t pieceIndex) : mPieceIndex(pieceIndex) {}

    std::vector<char> Have::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    bool operator==(const Have& h1, const Have& h2)
    {
        return h1.getPieceIndex() == h2.getPieceIndex();
    }

    std::ostream& operator<<(std::ostream& os, const Have& msg)
    {
        return os << "Have{PieceIndex: " << msg.getPieceIndex() << "}";
    }

    uint32_t Have::getPieceIndex() const
    {
        return mPieceIndex;
    }

    uint32_t Have::getLen() const
    {
        return MSG_LEN;
    }

    std::vector<char> Bitfield::getPrefix() const
    {
        auto result = common::int_to_bytes(getLen());
        result.push_back(MSG_TYPE);

        return result;
    }

    uint32_t Bitfield::getLen() const
    {
        return MSG_MIN_LEN + mBitfield.size();
    }

    const common::string_view Bitfield::getBitfield() const
    {
        return common::string_view(mBitfield.begin(), mBitfield.end());
    }

    bool operator==(const Bitfield& bf1, const Bitfield& bf2)
    {
        return bf1.getBitfield() == bf2.getBitfield();
    }

    std::ostream& operator<<(std::ostream& os, const Bitfield& msg)
    {
        return os << "Bitfield{Bitfield: " << msg.getBitfield() << "}";
    }

    Request::Request(uint32_t reqIndex, uint32_t reqBegin, uint32_t reqLen)
        : mReqIndex(reqIndex), mReqBegin(reqBegin), mReqLength(reqLen) {}

    std::vector<char> Request::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    uint32_t Request::getReqIndex() const
    {
        return mReqIndex;
    }

    uint32_t Request::getReqBegin() const
    {
        return mReqBegin;
    }

    uint32_t Request::getReqLength() const
    {
        return mReqLength;
    }

    uint32_t Request::getLen() const
    {
        return MSG_LEN;
    }

    bool operator==(const Request& rq1, const Request& rq2)
    {
        return rq1.getReqIndex() == rq2.getReqIndex()
            && rq1.getReqBegin() == rq2.getReqBegin()
            && rq1.getReqLength() == rq2.getReqLength();
    }

    std::ostream& operator<<(std::ostream& os, const Request& msg)
    {
        os << "Request{Index: " << msg.getReqIndex() << ", ";
        os << "Begin: " << msg.getReqBegin() << ", ";
        os << "Length: " << msg.getReqLength() << "}";

        return os;
    }

    std::vector<char> Piece::getPrefix() const
    {
        auto result = common::int_to_bytes(getLen());
        result.push_back(MSG_TYPE);

        return result;
    }

    uint32_t Piece::getLen() const
    {
        return MSG_MIN_LEN + mBlock.size();
    }

    uint32_t Piece::getPieceIndex() const
    {
        return mIndex;
    }

    uint32_t Piece::getPieceBegin() const
    {
        return mBegin;
    }

    const common::string_view Piece::getBlock() const
    {
        return common::string_view(mBlock.begin(), mBlock.end());
    }

    std::vector<char>&& Piece::extractBlock()
    {
        return std::move(mBlock);
    }

    bool operator==(const Piece& p1, const Piece& p2)
    {
        return p1.getPieceIndex() == p2.getPieceIndex()
            && p1.getPieceBegin() == p2.getPieceBegin()
            && p1.getBlock() == p2.getBlock();
    }

    std::ostream& operator<<(std::ostream& os, const Piece& msg)
    {
        os << "Piece{Index: " << msg.getPieceIndex() << ", ";
        os << "Begin: " << msg.getPieceBegin() << ", ";
        os << "Block: " << msg.getBlock() << "}";

        return os;
    }

    Cancel::Cancel(uint32_t index, uint32_t begin, uint32_t len) : mIndex(index), mBegin(begin), mLen(len) {}

    std::vector<char> Cancel::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    uint32_t Cancel::getCancelIndex() const
    {
        return mIndex;
    }

    uint32_t Cancel::getCancelBegin() const
    {
        return mBegin;
    }

    uint32_t Cancel::getCancelLength() const
    {
        return mLen;
    }

    uint32_t Cancel::getLen() const
    {
        return MSG_LEN;
    }

    bool operator==(const Cancel& c1, const Cancel& c2)
    {
        return c1.getCancelIndex() == c2.getCancelIndex()
            && c1.getCancelBegin() == c2.getCancelBegin()
            && c1.getCancelLength() == c2.getCancelLength();
    }

    std::ostream& operator<<(std::ostream& os, const Cancel& msg)
    {
        os << "Cancel{Index: " << msg.getCancelIndex() << ", ";
        os << "Begin: " << msg.getCancelBegin() << ", ";
        os << "Length: " << msg.getCancelLength() << "}";

        return os;
    }

    Port::Port(uint16_t port) : mPort(port) {}

    std::vector<char> Port::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    uint16_t Port::getPort() const
    {
        return mPort;
    }

    uint32_t Port::getLen() const
    {
        return MSG_LEN;
    }

    bool operator==(const Port& p1, const Port& p2)
    {
        return p1.getPort() == p2.getPort();
    }

    std::ostream& operator<<(std::ostream& os, const Port& msg)
    {
        return os << "Port{Port: " << msg.getPort() << "}";
    }

    std::vector<char> SerializeError::getPrefix() const
    {
        return {};
    }

    const common::string_view SerializeError::getBuffer() const
    {
        return common::string_view(mBufferedQueueManager.begin(), mBufferedQueueManager.end());
    }

    uint32_t SerializeError::getLen() const
    {
        return mError.size();
    }

    std::ostream& operator<<(std::ostream& os, const SerializeError& msg)
    {
        os << "SerializeError{MsgType: " << msg.msgType << ", ";
        os << "Error: " << msg.mError << ", ";
        os << "BufferedQueueManager: " << common::bytes_to_hex(msg.mBufferedQueueManager) << "}";
        return os;
    }

    uint32_t Disconnect::getLen() const
    {
        return 0;
    }
    
    std::vector<char> Disconnect::getPrefix() const
    {
        return {};
    }

    bool operator==(const Disconnect& lhs, const Disconnect& rhs)
    {
        return true;
    }

    std::ostream& operator<<(std::ostream& os, const Disconnect& msg)
    {
        return os << "Disconnect{}";
    }

    uint32_t Deactivate::getLen() const
    {
        return 0;
    }
    
    std::vector<char> Deactivate::getPrefix() const
    {
        return {};
    }

    bool operator==(const Deactivate& lhs, const Deactivate& rhs)
    {
        return true;
    }

    std::ostream& operator<<(std::ostream& os, const Deactivate& msg)
    {
        return os << "Deactivate{}";
    }
}