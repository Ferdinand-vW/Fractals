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

    std::vector<char> Choke::getPrefix() const
    {   
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    std::vector<char> UnChoke::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    std::vector<char> Interested::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    std::vector<char> NotInterested::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    std::vector<char> Have::getPrefix() const
    {
        auto result = common::int_to_bytes(MSG_LEN);
        result.push_back(MSG_TYPE);

        return result;
    }

    uint32_t Have::getPieceIndex() const
    {
        return mPieceIndex;
    }

    std::vector<char> Bitfield::getPrefix() const
    {
        auto result = common::int_to_bytes(getLen());
        result.push_back(MSG_TYPE);

        return result;
    }

    constexpr uint32_t Bitfield::getLen() const
    {
        return MSG_MIN_LEN + mBitfield.size();
    }

    const common::string_view Bitfield::getBitfield() const
    {
        return common::string_view(mBitfield.begin(), mBitfield.end());
    }

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

    std::vector<char> Piece::getPrefix() const
    {
        auto result = common::int_to_bytes(getLen());
        result.push_back(MSG_TYPE);

        return result;
    }

    constexpr uint32_t Piece::getLen() const
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
        return MSG_LENgth;
    }

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

    SerializeError::SerializeError(uint32_t msgType, std::vector<char> && buffer)
        : msgType(msgType), mBuffer(std::move(buffer)) {}

    SerializeError::SerializeError(uint32_t msgType, std::vector<char> && buffer, std::string&& reason)
        : msgType(msgType), mBuffer(std::move(buffer)), mError(std::move(reason)) {};

    SerializeError::SerializeError(std::vector<char> && buffer, std::string&& reason)
        : mBuffer(std::move(buffer)), mError(std::move(reason)) {};

    std::vector<char> SerializeError::getPrefix() const
    {
        return {};
    }

    const common::string_view SerializeError::getBuffer() const
    {
        return common::string_view(mBuffer.begin(), mBuffer.end());
    }

}