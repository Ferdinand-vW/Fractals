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
                    common::append(buf, common::intToBytes(t.getLen()));
                    common::append(buf, common::intToBytes(A::MSG_TYPE));
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
            BitTorrentMessage decodeHandShake(const Container& buf)
            {
                if (buf.size() <= 0)
                {
                    return SerializeError{buf, "Empty buffer"};
                }

                std::string_view view(buf.begin(), buf.end());

                uint32_t pstrLen = view.front();
                if (view.size() == pstrLen + 49)
                {
                    view.remove_prefix(1);                

                    return decodePayload<HandShake>(view, pstrLen);
                }

                return SerializeError{buf, "Cannot parse message as HandShake"};;
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
                std::string_view view(buf.begin(), buf.end());

                // Takes at most 4 bytes
                uint32_t len = common::bytesToInt<uint32_t>(view);

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
            BitTorrentMessage handleFailure(const std::optional<T> decoded, std::string_view& view)
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

            template <typename T>
            BitTorrentMessage decodePayload(std::string_view& buf, uint32_t len)
            {
                return handleFailure(decodePayloadImpl<T>(buf, len), buf);
            }

            template <typename T>
            std::optional<T> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<KeepAlive> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<HandShake> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<Choke> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<UnChoke> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<Interested> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<NotInterested> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<Have> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<Bitfield> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<Request> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<Piece> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<Cancel> decodePayloadImpl(std::string_view& buf, uint32_t len);
            template<>
            std::optional<Port> decodePayloadImpl(std::string_view& buf, uint32_t len);
    };
    
}