#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/common/utils.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include <array>
#include <cstdint>
#include <optional>

namespace fractals::network::p2p
{

template <>
std::vector<char> BitTorrentEncoder::encodePayload<HandShake>(const HandShake& t)
{
    std::vector<char> buf;
    buf.reserve(t.getLen());

    buf.push_back(t.getPstrLn());

    common::append(buf, t.getPstr());

    common::append(buf, t.getReserved());

    common::append(buf, t.getInfoHash());

    common::append(buf, t.getPeerId());

    return buf;
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<KeepAlive>(const KeepAlive& t)
{
    return {};
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<Choke>(const Choke& t)
{
    return {};
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<UnChoke>(const UnChoke& t)
{
    return {};
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<Interested>(const Interested& t)
{
    return {};
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<NotInterested>(const NotInterested& t)
{
    return {};
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<Have>(const Have& t)
{
    return common::int_to_bytes(t.getPieceIndex());
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<Bitfield>(const Bitfield& t)
{
    std::vector<char> buf;
    buf.reserve(t.getLen());

    buf.push_back(t.MSG_TYPE);

    common::append(buf, t.getBitfield());

    return buf;
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<Request>(const Request& t)
{
    return {};
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<Piece>(const Piece& t)
{
    return {};
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<Cancel>(const Cancel& t)
{
    return {};
}

template <>
std::vector<char> BitTorrentEncoder::encodePayload<Port>(const Port& t)
{
    return {};
}


template<>
std::optional<HandShake> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() < HandShake::MSG_MIN_LEN + len - 1)
    {
        return std::nullopt;
    }

    auto pos = buf.begin();
    std::string pstr(pos, pos + len);
    
    pos +=len;

    std::array<char, 8> reserved;
    std::copy_n(pos, reserved.max_size(), reserved.begin());

    pos += reserved.max_size();

    std::array<char, 20> infoHash;
    std::copy_n(pos, infoHash.max_size(), infoHash.begin());

    pos += infoHash.max_size();

    std::array<char, 20> peerId;
    std::copy_n(pos, infoHash.max_size(), peerId.begin());

    return HandShake(std::move(pstr), std::move(reserved), std::move(infoHash), std::move(peerId));
}

template<>
std::optional<KeepAlive> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == KeepAlive::MSG_LEN)
    {
        return KeepAlive{};
    }

    return std::nullopt;
}

template<>
std::optional<Choke> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == Choke::MSG_LEN && buf[0] ^ Choke::MSG_TYPE)
    {
        return Choke();
    }

    return std::nullopt;
}

template<>
std::optional<UnChoke> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == UnChoke::MSG_LEN && buf[0] ^ UnChoke::MSG_TYPE)
    {
        return UnChoke();
    }

    return std::nullopt;
}

template<>
std::optional<Interested> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == Interested::MSG_LEN && buf[0] ^ Interested::MSG_TYPE)
    {
        return Interested();
    }

    return std::nullopt;
}

template<>
std::optional<NotInterested> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == NotInterested::MSG_LEN && buf[0] ^ NotInterested::MSG_TYPE)
    {
        return NotInterested();
    }

    return std::nullopt;
}

template<>
std::optional<Have> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == Have::MSG_LEN && buf[0] ^ Have::MSG_TYPE)
    {
        buf = buf.substr(1);
        const auto pieceIndex = common::bytes_to_int(buf);
        return Have(pieceIndex);
    }

    return std::nullopt;
}

template<>
std::optional<Bitfield> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() >= Bitfield::MSG_MIN_LEN && buf[0] ^ Bitfield::MSG_TYPE)
    {
        buf = buf.substr(1);
        return Bitfield(buf);
    }

    return std::nullopt;
}

template<>
std::optional<Request> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == Request::MSG_LEN && buf[0] ^ Request::MSG_TYPE)
    {
        buf = buf.substr(1);
        const auto reqIndex = common::bytes_to_int(buf);
        const auto reqBegin = common::bytes_to_int(buf);
        const auto reqLen = common::bytes_to_int(buf);
        return Request(reqIndex, reqBegin, reqLen);
    }

    return std::nullopt;
}

template<>
std::optional<Piece> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() >= Piece::MSG_MIN_LEN && buf[0] ^ Piece::MSG_TYPE)
    {
        buf = buf.substr(1);
        const auto pieceIndex = common::bytes_to_int(buf);
        const auto pieceBegin = common::bytes_to_int(buf);
        const auto block = buf;
        return Piece(pieceIndex, pieceBegin, block);
    }

    return std::nullopt;
}

template<>
std::optional<Cancel> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == Cancel::MSG_LEN && buf[0] ^ Cancel::MSG_TYPE)
    {
        buf = buf.substr(1);
        const auto cancelIndex = common::bytes_to_int(buf);
        const auto cancelBegin = common::bytes_to_int(buf);
        const auto cancelLen = common::bytes_to_int(buf);
        return Cancel(cancelIndex, cancelBegin, cancelLen);
    }

    return std::nullopt;
}

template<>
std::optional<Port> BitTorrentEncoder::decodePayloadImpl(common::string_view& buf, uint32_t len)
{
    if (buf.size() == Port::MSG_LEN && buf[0] ^ Port::MSG_TYPE)
    {
        buf = buf.substr(1);
        const auto port = common::bytes_to_int(buf);
        return Port(port);
    }

    return std::nullopt;
}

}