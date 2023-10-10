cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)

# file(GLOB SOURCES "*.cpp" "")
set(SOURCES 
            fractals/app/Client.cpp
            fractals/app/Feedback.cpp
            fractals/app/ScreenTicker.cpp
            fractals/app/TorrentController.cpp
            fractals/app/TerminalInput.cpp
            fractals/app/TorrentDisplay.cpp
            fractals/app/TorrentDisplayEntry.cpp
            fractals/common/CurlPoll.cpp
            fractals/common/encode.cpp
            fractals/common/utils.cpp
            fractals/disk/IOLayer.cpp
            fractals/network/http/Announce.cpp
            fractals/network/http/AnnounceService.cpp
            fractals/network/http/Peer.cpp
            fractals/network/http/Request.cpp
            fractals/network/http/TrackerClient.cpp
            fractals/network/p2p/BitTorrentEncoder.cpp
            fractals/network/p2p/BitTorrentManager.cpp
            fractals/network/p2p/BitTorrentMsg.cpp
            fractals/network/p2p/BufferedQueueManager.cpp
            fractals/network/p2p/PeerEvent.cpp
            fractals/network/p2p/PieceStateManager.cpp
            fractals/network/p2p/Protocol.cpp
            fractals/network/p2p/Connection.cpp
            fractals/network/p2p/EpollServiceEvent.cpp
            fractals/network/p2p/EpollService.cpp
            fractals/network/p2p/EpollWrapper.cpp
            fractals/network/p2p/PeerManager.cpp
            fractals/network/p2p/PeerTracker.cpp
            fractals/network/p2p/Message.cpp 
            fractals/network/p2p/MessageType.cpp
            fractals/network/http/Peer.cpp
            fractals/torrent/Bencode.cpp
            fractals/torrent/TorrentMeta.cpp
            fractals/persist/PersistClient.cpp
            fractals/persist/PersistService.cpp
            fractals/torrent/File.cpp)

# Add source to this project's executable.
add_library(Fractals_lib ${SOURCES})
set_property(TARGET Fractals_lib PROPERTY ENABLE_EXPORTS ON)


target_include_directories(Fractals_lib PUBLIC ${NEITHER_INCLUDE_DIR})
target_link_libraries(Fractals_lib bencode)
target_link_libraries(Fractals_lib OpenSSL::SSL)
target_link_libraries(Fractals_lib ftxui::screen ftxui::dom ftxui::component)
target_link_libraries(Fractals_lib ${CURL_LIBRARIES})
target_link_libraries(Fractals_lib ${BOOST_LIBRARIES})
target_link_libraries(Fractals_lib ${EPOLL_WRAPPER_LIBRARIES})
target_link_libraries(Fractals_lib sqlite3)
target_link_libraries(Fractals_lib pthread dl backtrace)
target_link_libraries(Fractals_lib m)
# target_link_libraries(Fractals_lib asan)
target_link_libraries(Fractals_lib spdlog::spdlog)

add_executable (Fractals main.cpp)
target_link_libraries(Fractals ${EPOLL_WRAPPER_LIBRARIES})
target_link_libraries(Fractals Fractals_lib)
# target_link_libraries(Fractals asan)
# target_compile_options(Fractals PRIVATE -fno-omit-frame-pointer -fsanitize=address)
set_property(TARGET Fractals PROPERTY ENABLE_EXPORTS ON)
set_target_properties(Fractals PROPERTIES
            CXX_STANDARD 20
            CXX_EXTENSIONS OFF
            )
