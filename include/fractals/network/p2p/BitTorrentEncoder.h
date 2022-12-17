#pragma once

#include "BitTorrentMsg.h"
#include "fractals/common/utils.h"

#include <cstdint>
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
                return encodePayload<T>(t);
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

            template <typename Container>
            BitTorrentMessage decode(const Container& buf)
            {
                common::string_view view(buf.begin(), buf.end());

                if (buf.size() <= 0)
                {
                    return decodePayload<KeepAlive>(view);
                }

                uint32_t msgType = buf[0];

                switch(msgType)
                {
                    case Choke::MSG_TYPE: return decodePayload<Choke>(view);
                    case UnChoke::MSG_TYPE: return decodePayload<UnChoke>(view);
                    case Interested::MSG_TYPE: return decodePayload<Interested>(view);
                    case NotInterested::MSG_TYPE: return decodePayload<NotInterested>(view);
                    case Have::MSG_TYPE: return decodePayload<Have>(view);
                    case Bitfield::MSG_TYPE: return decodePayload<Bitfield>(view);
                    case Request::MSG_TYPE: return decodePayload<Request>(view);
                    case Piece::MSG_TYPE: return decodePayload<Piece>(view);
                    case Cancel::MSG_TYPE: return decodePayload<Cancel>(view);
                    case Port::MSG_TYPE: return decodePayload<Port>(view);
                    default:
                        return SerializeError{msgType, buf, "Cannot match parsed msgType to message"};
                }
            }

        private:
            template <typename T>
            BitTorrentMessage handleFailure(const std::optional<T> decoded, const common::string_view& view)
            {
                if (decoded)
                {
                    return *decoded;
                }
                else
                {
                    return SerializeError{std::vector(view.begin(), view.end()), "Failed to decode"};
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
            BitTorrentMessage decodePayload(const common::string_view& buf, int len = -1)
            {
                return handleFailure(decodePayloadImpl<T>(buf, len));
            }

            template <typename T>
            std::optional<T> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<HandShake> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<Choke> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<UnChoke> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<Interested> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<NotInterested> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<Have> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<Bitfield> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<Request> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<Piece> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<Cancel> decodePayloadImpl(const common::string_view& buf, int len);
            template<>
            std::optional<Port> decodePayloadImpl(const common::string_view& buf, int len);
    };
    
}