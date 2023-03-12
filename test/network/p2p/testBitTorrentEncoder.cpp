#include "fractals/common/utils.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BitTorrentMsg.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iterator>

namespace fractals::network::p2p
{

TEST(BT_ENCODER, HandShake)
{
    BitTorrentEncoder encoder;

    std::array<char, 20> infoHash = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                                     'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't'};
    std::array<char, 20> peerId = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                                   'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't'};
    HandShake hs{"abcde", {0, 1, 2, 3, 4, 5, 6, 7}, std::move(infoHash), std::move(peerId)};

    // clang-format off
    std::vector<char> bytes{
        5,
        'a','b','c','d','e' // peer identifier
        ,0,1,2,3,4,5,6,7, // reserved bytes
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t', // info hash
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t' // peer id
        };
    // clang-format on

    auto encoded = encoder.encode(hs);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    HandShake decoded = *encoder.decodeHandShake(view);
    ASSERT_EQ(decoded, hs);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeHandShake(encodedView), hs);
}

TEST(BT_ENCODER, KeepAlive)
{
    BitTorrentEncoder encoder;

    KeepAlive ka;

    std::vector<char> bytes{0, 0, 0, 0};

    auto encoded = encoder.encode(ka);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    KeepAlive decoded = *encoder.decodeOpt<KeepAlive>(view);
    ASSERT_EQ(decoded, ka);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<KeepAlive>(encodedView), ka);
}

TEST(BT_ENCODER, Choke)
{
    BitTorrentEncoder encoder;

    Choke choke;

    std::vector<char> bytes{0, 0, 0, 1, 0};

    auto encoded = encoder.encode(choke);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    Choke decoded = *encoder.decodeOpt<Choke>(view);
    ASSERT_EQ(decoded, choke);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Choke>(encodedView), choke);
}

TEST(BT_ENCODER, UnChoke)
{
    BitTorrentEncoder encoder;

    UnChoke ka;

    std::vector<char> bytes{0, 0, 0, 1, 1};

    auto encoded = encoder.encode(ka);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    UnChoke decoded = *encoder.decodeOpt<UnChoke>(view);
    ASSERT_EQ(decoded, ka);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<UnChoke>(encodedView), ka);
}

TEST(BT_ENCODER, Interested)
{
    BitTorrentEncoder encoder;

    Interested ka;

    std::vector<char> bytes{0, 0, 0, 1, 2};

    auto encoded = encoder.encode(ka);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    Interested decoded = *encoder.decodeOpt<Interested>(view);
    ASSERT_EQ(decoded, ka);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Interested>(encodedView), ka);
}

TEST(BT_ENCODER, NotInterested)
{
    BitTorrentEncoder encoder;

    NotInterested ka;

    std::vector<char> bytes{0, 0, 0, 1, 3};

    auto encoded = encoder.encode(ka);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    NotInterested decoded = *encoder.decodeOpt<NotInterested>(view);
    ASSERT_EQ(decoded, ka);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<NotInterested>(encodedView), ka);
}

TEST(BT_ENCODER, Have)
{
    BitTorrentEncoder encoder;

    Have have{1234};

    auto pieceAsBytes = common::int_to_bytes(1234);
    std::vector<char> bytes{0, 0, 0, 5, 4};
    bytes.insert(bytes.end(), pieceAsBytes.begin(), pieceAsBytes.end());

    auto encoded = encoder.encode(have);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    Have decoded = *encoder.decodeOpt<Have>(view);
    ASSERT_EQ(decoded, have);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Have>(encodedView), have);
}

TEST(BT_ENCODER, Bitfield)
{
    BitTorrentEncoder encoder;

    // 10 bits
    std::vector<bool> bits{0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0};
    const auto bitsAsBytes = common::bitfield_to_bytes(bits);
    Bitfield bf{bitsAsBytes};

    std::vector<char> bytes{0, 0, 0, static_cast<char>(bitsAsBytes.size() + 1), 5};
    // 10 bits results in 2 bytes
    bytes.insert(bytes.end(), bitsAsBytes.begin(), bitsAsBytes.end());

    auto encoded = encoder.encode(bf);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    Bitfield decoded = *encoder.decodeOpt<Bitfield>(view);
    ASSERT_EQ(decoded, bf);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Bitfield>(encodedView), bf);
}

TEST(BT_ENCODER, Request)
{
    BitTorrentEncoder encoder;

    Request rq(32, 1024, 200);

    const auto reqIndex = common::int_to_bytes(32);
    const auto reqBegin = common::int_to_bytes(1024);
    const auto reqLen = common::int_to_bytes(200);

    std::vector<char> bytes{0, 0, 0, 13, 6};
    bytes.insert(bytes.end(), reqIndex.begin(), reqIndex.end());
    bytes.insert(bytes.end(), reqBegin.begin(), reqBegin.end());
    bytes.insert(bytes.end(), reqLen.begin(), reqLen.end());

    auto encoded = encoder.encode(rq);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    Request decoded = *encoder.decodeOpt<Request>(view);
    ASSERT_EQ(decoded, rq);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Request>(encodedView), rq);
}

TEST(BT_ENCODER, Piece)
{
    BitTorrentEncoder encoder;

    const std::vector<char> block{'a', 'w', 'c', 'd', '1', 68, 111, 20, -20};
    Piece pc{32, 1024, block};

    const auto pcIndex = common::int_to_bytes(32);
    const auto pcBegin = common::int_to_bytes(1024);
    std::vector<char> bytes{0, 0, 0, 18, 7};
    bytes.insert(bytes.end(), pcIndex.begin(), pcIndex.end());
    bytes.insert(bytes.end(), pcBegin.begin(), pcBegin.end());
    bytes.insert(bytes.end(), block.begin(), block.end());

    auto encoded = encoder.encode(pc);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    Piece decoded = *encoder.decodeOpt<Piece>(view);
    ASSERT_EQ(decoded, pc);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Piece>(encodedView), pc);
}

TEST(BT_ENCODER, Cancel)
{
    BitTorrentEncoder encoder;

    Cancel cnl(32, 1024, 200);

    const auto cnlIndex = common::int_to_bytes(32);
    const auto cnlBegin = common::int_to_bytes(1024);
    const auto cnlLen = common::int_to_bytes(200);

    std::vector<char> bytes{0, 0, 0, 13, 8};
    bytes.insert(bytes.end(), cnlIndex.begin(), cnlIndex.end());
    bytes.insert(bytes.end(), cnlBegin.begin(), cnlBegin.end());
    bytes.insert(bytes.end(), cnlLen.begin(), cnlLen.end());

    auto encoded = encoder.encode(cnl);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    Cancel decoded = *encoder.decodeOpt<Cancel>(view);
    ASSERT_EQ(decoded, cnl);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Cancel>(encodedView), cnl);
}

TEST(BT_ENCODER, Port)
{
    BitTorrentEncoder encoder;

    Port prt(15000);

    const auto portNum = common::int_to_bytes<uint16_t>(15000);

    std::vector<char> bytes{0, 0, 0, 3, 9};
    bytes.insert(bytes.end(), portNum.begin(), portNum.end());

    auto encoded = encoder.encode(prt);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin(), bytes.end());
    Port decoded = *encoder.decodeOpt<Port>(view);
    ASSERT_EQ(decoded, prt);

    common::string_view encodedView(encoded.begin(), encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Port>(encodedView), prt);
}

} // namespace fractals::network::p2p