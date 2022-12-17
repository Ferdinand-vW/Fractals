#include "fractals/network/p2p/BitTorrentEncoder.h"
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

// template <>
// std::vector<char> encodePayload(const KeepAlive& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const Choke& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const UnChoke& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const Interested& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const NotInterested& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const Have& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const Bitfield& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const Request& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const Piece& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const Cancel& t, const std::vector<char>& buf);
// template <>
// std::vector<char> encodePayload(const Port& t, const std::vector<char>& buf);

template<>
std::optional<HandShake> BitTorrentEncoder::decodePayloadImpl(const common::string_view& buf, int len)
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

// template<>
// std::optional<Choke> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<UnChoke> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<Interested> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<NotInterested> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<Have> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<Bitfield> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<Request> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<Piece> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<Cancel> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

// template<>
// std::optional<Port> BitTorrentEncoder::decodePayload(const common::string_view& buf)
// {

// }

}