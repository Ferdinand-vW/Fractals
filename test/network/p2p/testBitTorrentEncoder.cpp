#include "fractals/common/utils.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BitTorrentMsg.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace fractals::network::p2p
{

TEST(BT_ENCODER, HandShake)
{
    BitTorrentEncoder encoder;

    std::array<char,20> infoHash = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t'};
    std::array<char,20> peerId = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t'};
    HandShake hs{"abcde", {0,1,2,3,4,5,6,7},std::move(infoHash), std::move(peerId)};
    
    std::vector<char> bytes{
        5,
        'a','b','c','d','e' // peer identifier
        ,0,1,2,3,4,5,6,7, // reserved bytes
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t', // info hash
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t' // peer id
        }; 

    auto encoded = encoder.encode(hs);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin()+1, bytes.end());
    HandShake decoded = *encoder.decodeHandShake(view, 5);
    ASSERT_EQ(decoded, hs);

    common::string_view encodedView(encoded.begin()+1, encoded.end());
    ASSERT_EQ(encoder.decodeHandShake(encodedView, encoded[0]), hs);
}

TEST(BT_ENCODER, KeepAlive)
{
    BitTorrentEncoder encoder;

    KeepAlive ka;

    std::vector<char> bytes{0,0,0,0}; 

    auto encoded = encoder.encode(ka);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin()+4, bytes.end());
    KeepAlive decoded = *encoder.decodeOpt<KeepAlive>(view, 4);
    ASSERT_EQ(decoded, ka);

    common::string_view encodedView(encoded.begin()+4, encoded.end());
    ASSERT_EQ(encoder.decodeOpt<KeepAlive>(encodedView, 4), ka);
}

TEST(BT_ENCODER, Choke)
{
    BitTorrentEncoder encoder;

    Choke choke;

    std::vector<char> bytes{0,0,0,0,1}; 

    auto encoded = encoder.encode(choke);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin()+4, bytes.end());
    Choke decoded = *encoder.decodeOpt<Choke>(view, 4);
    ASSERT_EQ(decoded, choke);

    common::string_view encodedView(encoded.begin()+4, encoded.end());
    ASSERT_EQ(encoder.decodeOpt<Choke>(encodedView, 4), choke);
}

TEST(BT_ENCODER, UnChoke)
{
    BitTorrentEncoder encoder;

    KeepAlive ka;

    std::vector<char> bytes{0,0,0,0}; 

    auto encoded = encoder.encode(ka);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin()+4, bytes.end());
    KeepAlive decoded = *encoder.decodeOpt<KeepAlive>(view, 4);
    ASSERT_EQ(decoded, ka);

    common::string_view encodedView(encoded.begin()+4, encoded.end());
    ASSERT_EQ(encoder.decodeOpt<KeepAlive>(encodedView, 4), ka);
}

TEST(BT_ENCODER, Interested)
{
    BitTorrentEncoder encoder;

    KeepAlive ka;

    std::vector<char> bytes{0,0,0,0}; 

    auto encoded = encoder.encode(ka);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin()+4, bytes.end());
    KeepAlive decoded = *encoder.decodeOpt<KeepAlive>(view, 4);
    ASSERT_EQ(decoded, ka);

    common::string_view encodedView(encoded.begin()+4, encoded.end());
    ASSERT_EQ(encoder.decodeOpt<KeepAlive>(encodedView, 4), ka);
}

TEST(BT_ENCODER, NotInterested)
{
    BitTorrentEncoder encoder;

    KeepAlive ka;

    std::vector<char> bytes{0,0,0,0}; 

    auto encoded = encoder.encode(ka);
    ASSERT_EQ(encoded, bytes);

    common::string_view view(bytes.begin()+4, bytes.end());
    KeepAlive decoded = *encoder.decodeOpt<KeepAlive>(view, 4);
    ASSERT_EQ(decoded, ka);

    common::string_view encodedView(encoded.begin()+4, encoded.end());
    ASSERT_EQ(encoder.decodeOpt<KeepAlive>(encodedView, 4), ka);
}

}