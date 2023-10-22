#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/common/utils.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace fractals::network::p2p
{

template <>
void BitTorrentEncoder::encodePayload<HandShake>(const HandShake& t, std::vector<char>& buf)
{
    buf.emplace_back(t.getPstrLn());

    common::append(buf, t.getPstr());

    common::append(buf, t.getReserved());

    common::append(buf, t.getInfoHash());

    common::append(buf, t.getPeerId());
}

template <>
void BitTorrentEncoder::encodePayload<KeepAlive>(const KeepAlive& t, std::vector<char>& buf)
{
    common::append(buf, common::intToBytes(t.getLen()));
}


template <>
void BitTorrentEncoder::encodePayload<Choke>(const Choke& t, std::vector<char>& buf)
{
}

template <>
void BitTorrentEncoder::encodePayload<UnChoke>(const UnChoke& t, std::vector<char>& buf)
{
}

template <>
void BitTorrentEncoder::encodePayload<Interested>(const Interested& t, std::vector<char>& buf)
{
}

template <>
void BitTorrentEncoder::encodePayload<NotInterested>(const NotInterested& t, std::vector<char>& buf)
{
}

template <>
void BitTorrentEncoder::encodePayload<Have>(const Have& t, std::vector<char>& buf)
{
    common::append(buf, common::intToBytes(t.getPieceIndex()));
}

template <>
void BitTorrentEncoder::encodePayload<Bitfield>(const Bitfield& t, std::vector<char>& buf)
{
    common::append(buf, t.getBitfield());
}

template <>
void BitTorrentEncoder::encodePayload<Request>(const Request& t, std::vector<char>& buf)
{
    common::append(buf, common::intToBytes(t.getReqIndex()));

    common::append(buf, common::intToBytes(t.getReqBegin()));

    common::append(buf, common::intToBytes(t.getReqLength()));
}

template <>
void BitTorrentEncoder::encodePayload<Piece>(const Piece& t, std::vector<char>& buf)
{
    common::append(buf, common::intToBytes(t.getPieceIndex()));

    common::append(buf, common::intToBytes(t.getPieceBegin()));

    common::append(buf, t.getBlock());
}

template <>
void BitTorrentEncoder::encodePayload<Cancel>(const Cancel& t, std::vector<char>& buf)
{
    common::append(buf, common::intToBytes(t.getCancelIndex()));

    common::append(buf, common::intToBytes(t.getCancelBegin()));

    common::append(buf, common::intToBytes(t.getCancelLength()));
}

template <>
void BitTorrentEncoder::encodePayload<Port>(const Port& t, std::vector<char>& buf)
{
    common::append(buf, common::intToBytes<uint16_t>(t.getPort()));
}


template <>
void BitTorrentEncoder::encodePayload<SerializeError>(const SerializeError& t, std::vector<char>& buf)
{
    throw std::invalid_argument("Cannot encode serialize error");
}

template<>
std::optional<HandShake> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t pstrLen)
{
    std::string pstr(buf.begin(), buf.begin() + pstrLen);
    
    buf.remove_prefix(pstrLen);

    std::array<char, 8> reserved;
    std::copy_n(buf.begin(), reserved.max_size(), reserved.begin());

    buf.remove_prefix(reserved.max_size());

    std::array<char, 20> infoHash;
    std::copy_n(buf.begin(), infoHash.max_size(), infoHash.begin());

    buf.remove_prefix(infoHash.max_size());

    std::array<char, 20> peerId;
    std::copy_n(buf.begin(), infoHash.max_size(), peerId.begin());

    return HandShake(std::move(pstr), std::move(reserved), std::move(infoHash), std::move(peerId));
}

template<>
std::optional<KeepAlive> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == KeepAlive::MSG_LEN)
    {
        return KeepAlive{};
    }

    return std::nullopt;
}

template<>
std::optional<Choke> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == Choke::MSG_LEN)
    {
        return Choke();
    }

    return std::nullopt;
}

template<>
std::optional<UnChoke> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == UnChoke::MSG_LEN)
    {
        return UnChoke();
    }

    return std::nullopt;
}

template<>
std::optional<Interested> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == Interested::MSG_LEN)
    {
        return Interested();
    }

    return std::nullopt;
}

template<>
std::optional<NotInterested> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == NotInterested::MSG_LEN)
    {
        return NotInterested();
    }

    return std::nullopt;
}

template<>
std::optional<Have> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == Have::MSG_LEN)
    {
        const auto pieceIndex = common::bytesToInt<uint32_t>(buf);
        return Have(pieceIndex);
    }

    return std::nullopt;
}

template<>
std::optional<Bitfield> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len >= Bitfield::MSG_MIN_LEN - 1)
    {
        return Bitfield(buf);
    }

    return std::nullopt;
}

template<>
std::optional<Request> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == Request::MSG_LEN)
    {
        const auto reqIndex = common::bytesToInt<uint32_t>(buf);
        const auto reqBegin = common::bytesToInt<uint32_t>(buf);
        const auto reqLen = common::bytesToInt<uint32_t>(buf);
        return Request(reqIndex, reqBegin, reqLen);
    }

    return std::nullopt;
}

template<>
std::optional<Piece> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len >= Piece::MSG_MIN_LEN)
    {
        const auto pieceIndex = common::bytesToInt<uint32_t>(buf);
        const auto pieceBegin = common::bytesToInt<uint32_t>(buf);
        const auto block = buf;
        return Piece(pieceIndex, pieceBegin, block);
    }

    return std::nullopt;
}

template<>
std::optional<Cancel> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == Cancel::MSG_LEN)
    {
        const auto cancelIndex = common::bytesToInt<uint32_t>(buf);
        const auto cancelBegin = common::bytesToInt<uint32_t>(buf);
        const auto cancelLen = common::bytesToInt<uint32_t>(buf);
        return Cancel(cancelIndex, cancelBegin, cancelLen);
    }

    return std::nullopt;
}

template<>
std::optional<Port> BitTorrentEncoder::decodePayloadImpl(std::string_view& buf, uint32_t len)
{
    if (len == Port::MSG_LEN)
    {
        const auto port = common::bytesToInt<uint16_t>(buf);
        return Port(port);
    }

    return std::nullopt;
}

}