#include "fractals/common/utils.h"
#include "fractals/network/http/Peer.h"
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

const PeerFd peer{http::PeerId{"",0}, Socket{0}};

TEST(BUFFER_MANAGER, HandShakeRead)
{
    PeerEventQueue wq;
    BufferedQueueManager btMan(wq);

    btMan.readPeerData<std::vector<char>>(peer, {4});

    auto *msgState = btMan.getReadBuffer(peer);

    ASSERT_TRUE(msgState);

    ASSERT_TRUE(msgState->isInitialized());
    ASSERT_FALSE(msgState->isComplete());

    // pstr
    btMan.readPeerData<std::vector<char>>(peer, {'a','b','c','d'});

    ASSERT_FALSE(msgState->isComplete());
    ASSERT_TRUE(wq.isEmpty());

    // reserved
    btMan.readPeerData<std::vector<char>>(peer, {1,2,3,4,5,6,7,8});

    ASSERT_FALSE(msgState->isComplete());
    ASSERT_TRUE(wq.isEmpty());

    // info hash
    btMan.readPeerData<std::vector<char>>(peer, {1,2,3,4,5,6,7,8,9,10});
    btMan.readPeerData<std::vector<char>>(peer, {1,2,3,4,5,6,7,8,9,10});

    ASSERT_FALSE(msgState->isComplete());
    ASSERT_TRUE(wq.isEmpty());

    // peer id
    btMan.readPeerData<std::vector<char>>(peer, {1,2,3,4,5,6,7,8,9,10});
    btMan.readPeerData<std::vector<char>>(peer, {1,2,3,4,5,6,7,8,9,10});

    ASSERT_FALSE(msgState->isInitialized()); // Resets msg after flush
    ASSERT_FALSE(wq.isEmpty());

}

TEST(BUFFER_MANAGER, HandShakeWrite)
{
    PeerEventQueue wq;
    BufferedQueueManager btMan(wq);

    std::string pstr{"abcd"};
    std::array<char,8> reserved {1,2,3,4,5,6,7,8};
    std::array<char,20> infoHash {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    std::array<char,20> peerId {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    HandShake hs(pstr,std::move(reserved), std::move(infoHash), std::move(peerId));
    btMan.sendToPeer(peer, hs);

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
    PeerEventQueue wq;
    BufferedQueueManager btMan(wq);

    // HandShake
    btMan.readPeerData<std::vector<char>>(peer, {0});

    auto *msgState = btMan.getReadBuffer(peer);

    btMan.readPeerData<std::vector<char>>(peer, std::vector<char>(48));

    ASSERT_FALSE(wq.isEmpty());
    wq.pop();

    btMan.readPeerData<std::vector<char>>(peer, {0});

    ASSERT_FALSE(msgState->isInitialized());

    btMan.readPeerData<std::vector<char>>(peer, {0});

    ASSERT_FALSE(msgState->isInitialized());

    ASSERT_TRUE(wq.isEmpty());

    btMan.readPeerData<std::vector<char>>(peer, {0,0});

    ASSERT_FALSE(msgState->isInitialized());

    ASSERT_FALSE(wq.isEmpty());

    auto event = wq.pop();

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(event));

    auto re = std::get<ReceiveEvent>(event);

    ASSERT_TRUE(std::holds_alternative<KeepAlive>(re.mMessage));
}

TEST(BUFFER_MANAGER, KeepAliveWrite)
{
    PeerEventQueue wq;
    BufferedQueueManager btMan(wq);

    KeepAlive ka;
    btMan.sendToPeer(peer, ka);

    auto *wmsg = btMan.getWriteBuffer(peer);

    ASSERT_TRUE(wmsg);
    
    auto &view = wmsg->getBuffer();

    EXPECT_THAT(view.substr(0, 4), testing::ElementsAre(0,0,0,0));

    wmsg->flush(4);

    ASSERT_TRUE(wmsg->isComplete());
}


TEST(BUFFER_MANAGER, BitfieldRead)
{
    PeerEventQueue wq;
    BufferedQueueManager btMan(wq);

    // HandShake
    btMan.readPeerData<std::vector<char>>(peer, {0});

    auto *msgState = btMan.getReadBuffer(peer);

    btMan.readPeerData(peer, std::vector<char>(48));

    ASSERT_FALSE(wq.isEmpty());
    wq.pop();

    btMan.readPeerData<std::vector<char>>(peer, {0});

    ASSERT_FALSE(msgState->isInitialized());

    btMan.readPeerData<std::vector<char>>(peer, {0});

    ASSERT_FALSE(msgState->isInitialized());

    ASSERT_TRUE(wq.isEmpty());

    btMan.readPeerData<std::vector<char>>(peer, {0,50});

    ASSERT_TRUE(msgState->isInitialized());

    btMan.readPeerData<std::vector<char>>(peer, {5}); // Bitfield type

    ASSERT_TRUE(wq.isEmpty());

    btMan.readPeerData(peer, std::vector<char>(20));

    ASSERT_TRUE(wq.isEmpty());

    btMan.readPeerData(peer, std::vector<char>(29));

    ASSERT_FALSE(wq.isEmpty());

    auto event = wq.pop();

    ASSERT_TRUE(std::holds_alternative<ReceiveEvent>(event));

    auto re = std::get<ReceiveEvent>(event);

    ASSERT_TRUE(std::holds_alternative<Bitfield>(re.mMessage));
}

TEST(BUFFER_MANAGER, BitfieldWrite)
{
    PeerEventQueue wq;
    BufferedQueueManager btMan(wq);

    http::PeerId p{"",0};

    Bitfield bf{"abcde"};
    btMan.sendToPeer(peer, bf);

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