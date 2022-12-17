#pragma once

#include <cstring>
#include <type_traits>

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <variant>

#include "fractals/network/p2p/MessageType.h"

#include "fractals/common/utils.h"

namespace fractals::network::p2p
{

struct HandShake;
struct KeepAlive;
struct Choke;
struct UnChoke;
struct Interested;
struct NotInterested;
struct Have;
struct Bitfield;
struct Request;
struct Piece;
struct Cancel;
struct Port;
struct SerializeError;

    using BitTorrentMessage =
        std::variant<Choke
                    ,UnChoke
                    ,Interested
                    ,NotInterested
                    ,Have
                    ,Bitfield
                    ,Request
                    ,Piece
                    ,Cancel
                    ,Port
                    ,KeepAlive
                    ,HandShake
                    ,SerializeError>;

    class HandShake
    {
        public:
            static constexpr uint32_t MSG_MIN_LEN = 49;
        public:
            HandShake() = default;
            HandShake(const std::string& pstr, std::array<char, 8>&& reserved,
                     std::array<char,20>&& infoHash, std::array<char, 20>&& peerId);
            std::vector<char> getPrefix() const;

            uint32_t getLen() const;
            char getPstrLn() const;
            const std::string& getPstr() const;
            const std::array<char, 8>& getReserved() const;
            const std::array<char, 20>& getInfoHash() const;
            const std::array<char, 20>& getPeerId() const;

            friend bool operator==(const HandShake&, const HandShake&);

        private:
            std::string mPstr;
            std::array<char,8> mReserved;
            std::array<char, 20> mInfoHash;
            std::array<char, 20> mPeerId;
    };

    class KeepAlive 
    {
        public:
            static constexpr uint32_t MSG_LEN = 1;
        public:
            constexpr uint32_t getLen() const;
            std::vector<char> getPrefix() const;
    };

    class Choke 
    {
        public:
            constexpr uint32_t getLen() const;
            static constexpr uint32_t MSG_LEN = 1;
            static constexpr uint32_t MSG_TYPE = 0;

        public:
            std::vector<char> getPrefix() const;
    };

    class UnChoke 
    {
        public:
            constexpr uint32_t getLen() const;
            static constexpr uint32_t MSG_LEN = 1;
            static constexpr uint32_t MSG_TYPE = 1;

        public:
            std::vector<char> getPrefix() const;
    };

    class Interested 
    {
        public:
            constexpr uint32_t getLen() const;
            static constexpr uint32_t MSG_LEN = 1;
            static constexpr uint32_t MSG_TYPE = 2;

        public:
            std::vector<char> getPrefix() const;
    };

    class NotInterested 
    {
        public:
            static constexpr uint32_t MSG_LEN = 1;
            static constexpr uint32_t MSG_TYPE = 3;

        public:
            constexpr uint32_t getLen() const;
            std::vector<char> getPrefix() const;
    };

    class Have 
    {
        public:
            static constexpr uint32_t MSG_LEN = 5;
            static constexpr uint32_t MSG_TYPE = 4;

        public:
            constexpr uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getPieceIndex() const;
        
        private:
            uint32_t mPieceIndex;

    };

    class Bitfield 
    {
        public:
            static constexpr uint32_t MSG_MIN_LEN = 1;
            static constexpr uint32_t MSG_TYPE = 5;

        public:
            constexpr uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            const common::string_view getBitfield() const;

        private:
            std::vector<char> mBitfield;
    };

    class Request 
    {
        public:
            static constexpr uint32_t MSG_LEN = 13;
            static constexpr uint32_t MSG_TYPE = 6;

        public:
            constexpr uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getReqIndex() const;
            uint32_t getReqBegin() const;
            uint32_t geReqtLength() const;

        private:
            uint32_t mReqIndex;
            uint32_t mReqBegin;
            uint32_t mReqLength;
    };

    class Piece 
    {
        public:
            static constexpr uint32_t MSG_MIN_LEN = 9;
            static constexpr uint32_t MSG_TYPE = 7;

        public:
            constexpr uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getPieceIndex() const;
            uint32_t getPieceBegin() const;
            const common::string_view getBlock() const;

        private:
            uint32_t mIndex;
            uint32_t mBegin;
            std::vector<char> mBlock;
    };

    class Cancel 
    {
        public:
            static constexpr uint32_t MSG_LEN = 13;
            static constexpr uint32_t MSG_TYPE = 8;

        public:
            constexpr uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getCancelIndex() const;
            uint32_t getCancelBegin() const;
            uint32_t getCancelLength() const;

        private:
            uint32_t mIndex;
            uint32_t mBegin;
            uint32_t MSG_LENgth;
    };

    class Port 
    {
        public:
            static constexpr uint32_t MSG_TYPE = 9;
            static constexpr uint32_t MSG_LEN = 3;

        public:
            constexpr uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getPort() const;
        
        private:
            uint16_t mPort;
    };

    class SerializeError
    {
        public:
            SerializeError() = default;
            SerializeError(uint32_t msgType, std::vector<char> && buffer);
            SerializeError(uint32_t msgType, std::vector<char> && buffer, std::string&& reason);
            SerializeError(std::vector<char> && buffer, std::string&& reason);

            std::vector<char> getPrefix() const;
            const common::string_view getBuffer() const;
        
        private:
            int32_t msgType{-1};
            std::string mError{""};
            std::vector<char> mBuffer;
    };
}