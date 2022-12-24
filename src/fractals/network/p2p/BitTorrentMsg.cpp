#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/common/utils.h"
#include <utility>

namespace fractals::network::p2p
{
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

    std::vector<char> KeepAlive::getPrefix() const
    {
        return common::int_to_bytes(MSG_LEN);
    }

    bool operator==(const KeepAlive&, const KeepAlive&)
    {
        return true;
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

    uint32_t Request::geReqtLength() const
    {
        return mReqLength;
    }

    uint32_t Request::getLen() const
    {
        return MSG_LEN;
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

    Port::Port(uint16_t port) : mPort(port) {}

    std::vector<char> Port::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    uint32_t Port::getPort() const
    {
        return mPort;
    }

    uint32_t Port::getLen() const
    {
        return MSG_LEN;
    }

    std::vector<char> SerializeError::getPrefix() const
    {
        return {};
    }

    const common::string_view SerializeError::getBuffer() const
    {
        return common::string_view(mBuffer.begin(), mBuffer.end());
    }

    uint32_t SerializeError::getLen() const
    {
        return mError.size();
    }

}