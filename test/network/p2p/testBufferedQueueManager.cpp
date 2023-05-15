#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
#include "fractals/network/p2p/BitTorrentEncoder.h"
#include "fractals/network/p2p/BufferedQueueManager.h"
#include "fractals/network/p2p/BitTorrentMsg.h"
#include "fractals/network/p2p/PeerFd.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iterator>
#include <variant>
#include <vector>

namespace fractals::network::p2p
{

const PeerFd peer{http::PeerId{"",0}, 0};
BitTorrentEncoder encoder;

TEST(BUFFER_MANAGER, HandShakeRead)
{
    BufferedQueueManager btMan;

    btMan.addToReadBuffer<std::vector<char>>(peer, {4});

    auto *msgState = btMan.getReadBuffer(peer);

    ASSERT_TRUE(msgState);

    ASSERT_TRUE(msgState->isInitialized());
    ASSERT_FALSE(msgState->isComplete());

    // pstr
    btMan.addToReadBuffer<std::vector<char>>(peer, {'a','b','c','d'});

    ASSERT_FALSE(msgState->isComplete());

    // reserved
    btMan.addToReadBuffer<std::vector<char>>(peer, {1,2,3,4,5,6,7,8});

    ASSERT_FALSE(msgState->isComplete());

    // info hash
    btMan.addToReadBuffer<std::vector<char>>(peer, {1,2,3,4,5,6,7,8,9,10});
    btMan.addToReadBuffer<std::vector<char>>(peer, {1,2,3,4,5,6,7,8,9,10});

    ASSERT_FALSE(msgState->isComplete());

    // peer id
    btMan.addToReadBuffer<std::vector<char>>(peer, {1,2,3,4,5,6,7,8,9,10});
    bool complete = btMan.addToReadBuffer<std::vector<char>>(peer, {1,2,3,4,5,6,7,8,9,10});

    ASSERT_TRUE(complete);
    ASSERT_TRUE(msgState->isComplete());
    auto btMsg = encoder.decodeHandShake(msgState->getBuffer());
    std::cout << "";
    ASSERT_TRUE(btMsg);
}

TEST(BUFFER_MANAGER, HandShakeWrite)
{
    BufferedQueueManager btMan;

    std::string pstr{"abcd"};
    std::array<char,8> reserved {1,2,3,4,5,6,7,8};
    std::array<char,20> infoHash {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    std::array<char,20> peerId {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    HandShake hs(pstr,std::move(reserved), std::move(infoHash), std::move(peerId));
    btMan.addToWriteBuffer(peer, encoder.encode(hs));

    auto *wmsg = btMan.getWriteBuffer(peer);

    ASSERT_TRUE(wmsg);
    
    auto &view = wmsg->getBuffer();

    char len = view.front();
    ASSERT_EQ(len, pstr.size());

    wmsg->flush(1);

    ASSERT_NE(view.front(), 4);

    ASSERT_EQ(view.substr(0, 4), pstr);

    wmsg->flush(4);

    common::string_view reservedView(reserved.begin(), reserved.end());
    EXPECT_THAT(view.substr(0, 8), testing::ContainerEq(reservedView));

    wmsg->flush(8);

    common::string_view infoHashView(infoHash.begin(), infoHash.end());
    EXPECT_THAT(view.substr(0, 20), testing::ContainerEq(infoHashView));

    wmsg->flush(20);

    common::string_view peerIdView(peerId.begin(), peerId.end());
    EXPECT_THAT(view, testing::ContainerEq(peerIdView));

    wmsg->flush(20);

    ASSERT_TRUE(wmsg->isComplete());
}

TEST(BUFFER_MANAGER, KeepAliveRead)
{
    BufferedQueueManager btMan;

    // HandShake
    btMan.addToReadBuffer<std::vector<char>>(peer, {0});

    auto &deq = btMan.getReadBuffers(peer);

    bool completed = btMan.addToReadBuffer<std::vector<char>>(peer, std::vector<char>(48));
    ASSERT_TRUE(completed);
    ASSERT_TRUE(encoder.decodeHandShake(deq.back().getBuffer()));
    deq.pop_back();

    btMan.addToReadBuffer<std::vector<char>>(peer, {0});
    ASSERT_FALSE(deq.back().isInitialized());

    btMan.addToReadBuffer<std::vector<char>>(peer, {0});
    ASSERT_FALSE(deq.back().isInitialized());


    btMan.addToReadBuffer<std::vector<char>>(peer, {0,0});
    ASSERT_TRUE(deq.back().isInitialized());
    ASSERT_TRUE(deq.back().isComplete());
    ASSERT_TRUE(std::holds_alternative<KeepAlive>(encoder.decode(deq.back().getBuffer())));
}

TEST(BUFFER_MANAGER, KeepAliveWrite)
{
    BufferedQueueManager btMan;

    btMan.addToWriteBuffer(peer, encoder.encode(KeepAlive{}));

    auto *wmsg = btMan.getWriteBuffer(peer);

    ASSERT_TRUE(wmsg);
    
    auto &view = wmsg->getBuffer();

    EXPECT_THAT(view.substr(0, 4), testing::ElementsAre(0,0,0,0));

    wmsg->flush(4);

    ASSERT_TRUE(wmsg->isComplete());
}

TEST(BUFFER_MANAGER, KeepAliveWriteThrice)
{
    // PeerEventQueue wq;
    BufferedQueueManager btMan;

    KeepAlive ka;
    
    for(int i = 0; i < 3; ++i)
    {
        btMan.addToWriteBuffer(peer, encoder.encode(ka));

        auto *wmsg = btMan.getWriteBuffer(peer);

        ASSERT_TRUE(wmsg);
        ASSERT_FALSE(wmsg->isComplete());
        
        auto &view = wmsg->getBuffer();

        EXPECT_THAT(view.substr(0, 4), testing::ElementsAre(0,0,0,0));

        wmsg->flush(4);

        ASSERT_TRUE(wmsg->isComplete());
    }
}

TEST(BUFFER_MANAGER, BitfieldRead)
{
    BufferedQueueManager btMan;

    // HandShake
    btMan.addToReadBuffer<std::vector<char>>(peer, {0});

    auto &deq = btMan.getReadBuffers(peer);

    bool completed = btMan.addToReadBuffer(peer, std::vector<char>(48));

    ASSERT_TRUE(completed);
    ASSERT_TRUE(encoder.decodeHandShake(deq.back().getBuffer()));
    // deq.pop_back();

    btMan.addToReadBuffer<std::vector<char>>(peer, {0});

    ASSERT_FALSE(deq.back().isInitialized());

    btMan.addToReadBuffer<std::vector<char>>(peer, {0});

    ASSERT_FALSE(deq.back().isInitialized());

    btMan.addToReadBuffer<std::vector<char>>(peer, {0,50});

    ASSERT_TRUE(deq.back().isInitialized());

    btMan.addToReadBuffer<std::vector<char>>(peer, {5}); // Bitfield type

    btMan.addToReadBuffer(peer, std::vector<char>(20));

    completed = btMan.addToReadBuffer(peer, std::vector<char>(29));
    ASSERT_TRUE(completed);

    const auto msg = encoder.decode(deq.back().getBuffer());
    ASSERT_TRUE(std::holds_alternative<Bitfield>(msg));
}

TEST(BUFFER_MANAGER, BitfieldWrite)
{
    BufferedQueueManager btMan;

    http::PeerId p{"",0};

    Bitfield bf{"abcde"};
    btMan.addToWriteBuffer(peer, encoder.encode(bf));

    auto *wmsg = btMan.getWriteBuffer(peer);

    ASSERT_TRUE(wmsg);
    
    auto &view = wmsg->getBuffer();

    EXPECT_THAT(view.substr(0, 4), testing::ElementsAre(0,0,0,6));

    wmsg->flush(4);

    EXPECT_THAT(view.substr(0, 1), testing::ElementsAre(5));

    wmsg->flush(1);

    EXPECT_THAT(view.substr(0, 5), testing::ElementsAre('a','b','c','d','e'));

    wmsg->flush(5);

    ASSERT_TRUE(wmsg->isComplete());
}

}