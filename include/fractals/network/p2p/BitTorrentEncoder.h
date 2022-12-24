#pragma once

#include "BitTorrentMsg.h"
#include "fractals/common/utils.h"

#include <cstdint>
#include <optional>
#include <type_traits>
#include <vector>
#include <variant>
#include <cstring>
#include <iostream>


namespace fractals::network::p2p
{
    class BitTorrentEncoder
    {
        public:
            template <typename T>
            std::vector<char> encode(const T& t)
            {
                using A = std::decay_t<T>;
                if constexpr (A::MSG_TYPE >= 0)
                {
                    std::vector<char> buf;
                    buf.reserve(t.getLen());
                    common::append(buf, common::int_to_bytes(t.getLen()));
                    common::append(buf, common::int_to_bytes(A::MSG_TYPE));
                    common::append(buf, encodePayload(t));
                    return buf;
                }
                else
                {
                    std::vector<char> buf;
                    buf.reserve(t.getLen());
                    common::append(buf, encodePayload(t));
                    return buf;
                }
            }

            std::vector<char> encode(const BitTorrentMessage &m)
            {
                return std::visit(common::overloaded {
                    [this](const auto& m){ return encode<decltype(m)>(m); }
                }, m);
            }

            template <typename Container>
            std::optional<HandShake> decodeHandShake(const Container& buf, char pstrLen)
            {
                common::string_view view(buf.begin(), buf.end());

                return decodePayloadImpl<HandShake>(view, pstrLen);
            }

            template <typename A, typename Container>
            std::optional<A> decodeOpt(const Container& buf, uint32_t len)
            {
                const auto btm = decode<Container>(buf, len);

                if (std::holds_alternative<A>(btm))
                {
                    return std::get<A>(btm);
                }

                return std::nullopt;
            }

            template <typename Container>
            BitTorrentMessage decode(const Container& buf, uint32_t len)
            {
                common::string_view view(buf.begin(), buf.end());

                if (buf.size() <= 0)
                {
                    return decodePayload<KeepAlive>(view, len);
                }

                uint32_t msgType = buf[0];

                switch(msgType)
                {
                    case Choke::MSG_TYPE: return decodePayload<Choke>(view, len);
                    case UnChoke::MSG_TYPE: return decodePayload<UnChoke>(view, len);
                    case Interested::MSG_TYPE: return decodePayload<Interested>(view, len);
                    case NotInterested::MSG_TYPE: return decodePayload<NotInterested>(view, len);
                    case Have::MSG_TYPE: return decodePayload<Have>(view, len);
                    case Bitfield::MSG_TYPE: return decodePayload<Bitfield>(view, len);
                    case Request::MSG_TYPE: return decodePayload<Request>(view, len);
                    case Piece::MSG_TYPE: return decodePayload<Piece>(view, len);
                    case Cancel::MSG_TYPE: return decodePayload<Cancel>(view, len);
                    case Port::MSG_TYPE: return decodePayload<Port>(view, len);
                    default:
                        return SerializeError{msgType, buf, "Cannot match parsed msgType to message"};
                }
            }

        private:
            template <typename T>
            BitTorrentMessage handleFailure(const std::optional<T> decoded, common::string_view& view)
            {
                if (decoded)
                {
                    return *decoded;
                }
                else
                {
                    return SerializeError{view,  "Failed to decode"};
                }
            }

            template <typename T>
            std::vector<char> encodePayload(const T& t);
            template <>
            std::vector<char> encodePayload(const HandShake& t);
            template <>
            std::vector<char> encodePayload(const KeepAlive& t);
            template <>
            std::vector<char> encodePayload(const Choke& t);
            template <>
            std::vector<char> encodePayload(const UnChoke& t);
            template <>
            std::vector<char> encodePayload(const Interested& t);
            template <>
            std::vector<char> encodePayload(const NotInterested& t);
            template <>
            std::vector<char> encodePayload(const Have& t);
            template <>
            std::vector<char> encodePayload(const Bitfield& t);
            template <>
            std::vector<char> encodePayload(const Request& t);
            template <>
            std::vector<char> encodePayload(const Piece& t);
            template <>
            std::vector<char> encodePayload(const Cancel& t);
            template <>
            std::vector<char> encodePayload(const Port& t);

            template <typename T>
            BitTorrentMessage decodePayload(common::string_view& buf, uint32_t len = -1)
            {
                return handleFailure(decodePayloadImpl<T>(buf, len), buf);
            }

            template <typename T>
            std::optional<T> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<KeepAlive> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<HandShake> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<Choke> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<UnChoke> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<Interested> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<NotInterested> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<Have> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<Bitfield> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<Request> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<Piece> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<Cancel> decodePayloadImpl(common::string_view& buf, uint32_t len);
            template<>
            std::optional<Port> decodePayloadImpl(common::string_view& buf, uint32_t len);
    };
    
}