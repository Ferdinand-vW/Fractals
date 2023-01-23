#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <variant>

#include "fractals/common/WorkQueue.h"
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
            static constexpr int8_t MSG_TYPE = -1;
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
            friend std::ostream& operator<<(std::ostream& os, const HandShake& msg);

        private:
            std::string mPstr;
            std::array<char,8> mReserved;
            std::array<char, 20> mInfoHash;
            std::array<char, 20> mPeerId;
    };

    class KeepAlive 
    {
        public:
            static constexpr int8_t MSG_TYPE = -1;
            static constexpr uint32_t MSG_LEN = 0;
        public:
            uint32_t getLen() const;
            std::vector<char> getPrefix() const;

            friend bool operator==(const KeepAlive&, const KeepAlive&);
            friend std::ostream& operator<<(std::ostream& os, const KeepAlive& msg);
    };

    class Choke 
    {
        public:
            uint32_t getLen() const;
            static constexpr uint32_t MSG_LEN = 1;
            static constexpr int8_t MSG_TYPE = 0;

            friend bool operator==(const Choke&, const Choke&);
            friend std::ostream& operator<<(std::ostream& os, const Choke& msg);

        public:
            std::vector<char> getPrefix() const;
    };

    class UnChoke 
    {
        public:
            uint32_t getLen() const;
            static constexpr uint32_t MSG_LEN = 1;
            static constexpr int8_t MSG_TYPE = 1;

        public:
            std::vector<char> getPrefix() const;

            friend bool operator==(const UnChoke&, const UnChoke&);
            friend std::ostream& operator<<(std::ostream& os, const UnChoke& msg);
    };

    class Interested 
    {
        public:
            uint32_t getLen() const;
            static constexpr uint32_t MSG_LEN = 1;
            static constexpr int8_t MSG_TYPE = 2;

        public:
            std::vector<char> getPrefix() const;

            friend bool operator==(const Interested&, const Interested&);
            friend std::ostream& operator<<(std::ostream& os, const Interested& msg);
    };

    class NotInterested 
    {
        public:
            static constexpr uint32_t MSG_LEN = 1;
            static constexpr int8_t MSG_TYPE = 3;

        public:
            uint32_t getLen() const;
            std::vector<char> getPrefix() const;

            friend bool operator==(const NotInterested&, const NotInterested&);
            friend std::ostream& operator<<(std::ostream& os, const NotInterested& msg);
    };

    class Have 
    {
        public:
            static constexpr uint32_t MSG_LEN = 5;
            static constexpr int8_t MSG_TYPE = 4;

        public:
            Have() = default;
            Have(uint32_t pieceIndex);

            uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getPieceIndex() const;

            friend bool operator==(const Have&, const Have&);
            friend std::ostream& operator<<(std::ostream& os, const Have& msg);
        
        private:
            uint32_t mPieceIndex;

    };

    class Bitfield 
    {
        public:
            static constexpr uint32_t MSG_MIN_LEN = 1;
            static constexpr int8_t MSG_TYPE = 5;

        public:
            Bitfield() = default;
            explicit Bitfield(const std::vector<char>& c) : mBitfield(c.begin(), c.end()) {}
            explicit Bitfield(const common::string_view& c) : mBitfield(c.begin(), c.end()) {}
            

            uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            const common::string_view getBitfield() const;

            friend bool operator==(const Bitfield&, const Bitfield&);
            friend std::ostream& operator<<(std::ostream& os, const Bitfield& msg);

        private:
            std::vector<char> mBitfield;
    };

    class Request 
    {
        public:
            static constexpr uint32_t MSG_LEN = 13;
            static constexpr int8_t MSG_TYPE = 6;

        public:
            Request() = default;
            Request(uint32_t reqIndex, uint32_t reqBegin, uint32_t reqLen);

            uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getReqIndex() const;
            uint32_t getReqBegin() const;
            uint32_t getReqLength() const;

            friend bool operator==(const Request&, const Request&);
            friend std::ostream& operator<<(std::ostream& os, const Request& msg);

        private:
            uint32_t mReqIndex;
            uint32_t mReqBegin;
            uint32_t mReqLength;
    };

    class Piece 
    {
        public:
            static constexpr uint32_t MSG_MIN_LEN = 9;
            static constexpr int8_t MSG_TYPE = 7;

        public:
            Piece() = default;
            template <typename Container>
            Piece(uint32_t pieceIndex, uint32_t pieceBegin, Container&& block) 
                : mIndex(pieceIndex), mBegin(pieceBegin), mBlock(block.begin(), block.end()) {}

            uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getPieceIndex() const;
            uint32_t getPieceBegin() const;
            const common::string_view getBlock() const;

            std::vector<char>&& extractBlock();

            friend bool operator==(const Piece&, const Piece&);
            friend std::ostream& operator<<(std::ostream& os, const Piece& msg);

        private:
            uint32_t mIndex;
            uint32_t mBegin;
            std::vector<char> mBlock;
    };

    class Cancel 
    {
        public:
            static constexpr uint32_t MSG_LEN = 13;
            static constexpr int8_t MSG_TYPE = 8;

        public:
            Cancel() = default;
            Cancel(uint32_t index, uint32_t begin, uint32_t len);

            uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint32_t getCancelIndex() const;
            uint32_t getCancelBegin() const;
            uint32_t getCancelLength() const;

            friend bool operator==(const Cancel&, const Cancel&);
            friend std::ostream& operator<<(std::ostream& os, const Cancel& msg);

        private:
            uint32_t mIndex;
            uint32_t mBegin;
            uint32_t mLen;
    };

    class Port 
    {
        public:
            static constexpr int8_t MSG_TYPE = 9;
            static constexpr uint32_t MSG_LEN = 3;

        public:
            Port() = default;
            Port(uint16_t port);

            uint32_t getLen() const;
            std::vector<char> getPrefix() const;
            uint16_t getPort() const;

            friend bool operator==(const Port&, const Port&);
            friend std::ostream& operator<<(std::ostream& os, const Port& msg);
        
        private:
            uint16_t mPort;
    };

    class SerializeError
    {
        public:
            static constexpr int8_t MSG_TYPE = -1;

            SerializeError() = default;
            template <typename Container>
            SerializeError(uint32_t msgType, Container && buffer)
                : msgType(msgType), mBufferedQueueManager(buffer.begin(), buffer.end()) {}
            
            template <typename Container>
            SerializeError(uint32_t msgType, Container && buffer, std::string&& reason)
                : msgType(msgType), mBufferedQueueManager(buffer.begin(), buffer.end()), mError(std::move(reason)) {}
            template <typename Container>
            SerializeError(Container && buffer, std::string&& reason)
                : mBufferedQueueManager(buffer.begin(), buffer.end()), mError(std::move(reason)) {}

            std::vector<char> getPrefix() const;
            const common::string_view getBuffer() const;
            uint32_t getLen() const;

            friend std::ostream& operator<<(std::ostream& os, const SerializeError& msg);
        
        private:
            int32_t msgType{-1};
            std::string mError{""};
            std::vector<char> mBufferedQueueManager;
    };

    std::ostream& operator<<(std::ostream& os, const BitTorrentMessage& msg);


    using BitTorrentMsgQueue = common::WorkQueueImpl<256, BitTorrentMessage>;
}