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
                    encodePayload(t, buf);
                    return buf;
                }
                else
                {
                    std::vector<char> buf;
                    buf.reserve(t.getLen());
                    encodePayload(t, buf);
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
            std::optional<HandShake> decodeHandShake(const Container& buf)
            {
                if (buf.size() <= 0)
                {
                    return std::nullopt;
                }

                common::string_view view(buf.begin(), buf.end());

                uint32_t pstrLen = view.front();
                view.remove_prefix(1);

                return decodePayloadImpl<HandShake>(view, pstrLen);
            }

            template <typename A, typename Container>
            std::optional<A> decodeOpt(const Container& buf)
            {
                const auto btm = decode<Container>(buf);

                if (std::holds_alternative<A>(btm))
                {
                    return std::get<A>(btm);
                }

                return std::nullopt;
            }

            template <typename Container>
            BitTorrentMessage decode(const Container& buf)
            {
                common::string_view view(buf.begin(), buf.end());

                // Takes at most 4 bytes
                uint32_t len = common::bytes_to_int<uint32_t>(view);

                if (len != view.size())
                {
                    return SerializeError{buf, "Parsed length does not match buffer size"};
                }

                if (len == 0 && view.size() == 0)
                {
                    return decodePayload<KeepAlive>(view, len);
                }

                uint32_t msgType = view.front();
                view.remove_prefix(1);

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
            void encodePayload(const T& t, std::vector<char>& buf);
            template <>
            void encodePayload(const HandShake& t, std::vector<char>& buf);
            template <>
            void encodePayload(const KeepAlive& t, std::vector<char>& buf);
            template <>
            void encodePayload(const Choke& t, std::vector<char>& buf);
            template <>
            void encodePayload(const UnChoke& t, std::vector<char>& buf);
            template <>
            void encodePayload(const Interested& t, std::vector<char>& buf);
            template <>
            void encodePayload(const NotInterested& t, std::vector<char>& buf);
            template <>
            void encodePayload(const Have& t, std::vector<char>& buf);
            template <>
            void encodePayload(const Bitfield& t, std::vector<char>& buf);
            template <>
            void encodePayload(const Request& t, std::vector<char>& buf);
            template <>
            void encodePayload(const Piece& t, std::vector<char>& buf);
            template <>
            void encodePayload(const Cancel& t, std::vector<char>& buf);
            template <>
            void encodePayload(const Port& t, std::vector<char>& buf);

            template<>
            void encodePayload(const Disconnect& d, std::vector<char>& buf) {}
            template<>
            void encodePayload(const Deactivate& d, std::vector<char>& buf) {}

            template <typename T>
            BitTorrentMessage decodePayload(common::string_view& buf, uint32_t len)
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