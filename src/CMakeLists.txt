cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

# file(GLOB SOURCES "*.cpp" "")
set(SOURCES fractals/app/Client.cpp
            fractals/app/Feedback.cpp
            fractals/app/ScreenTicker.cpp
            fractals/app/TorrentController.cpp
            fractals/app/TerminalInput.cpp
            fractals/app/TorrentDisplay.cpp
            fractals/app/TorrentView.cpp
            fractals/common/encode.cpp
            fractals/common/utils.cpp
            fractals/network/http/Announce.cpp
            fractals/network/http/Peer.cpp
            fractals/network/http/Request.cpp
            fractals/network/http/Tracker.cpp
            fractals/network/p2p/AnnounceService.cpp
            fractals/network/p2p/Connection.cpp
            fractals/network/p2p/Event.cpp
            fractals/network/p2p/PeerManager.cpp
            fractals/network/p2p/BitTorrent.cpp
            fractals/network/p2p/Client.cpp
            fractals/network/p2p/Message.cpp 
            fractals/network/p2p/MessageType.cpp
            fractals/network/http/Peer.cpp
            fractals/torrent/Bencode.cpp
            fractals/torrent/File.cpp
            fractals/torrent/Piece.cpp
            fractals/torrent/Torrent.cpp
            fractals/torrent/TorrentMeta.cpp
            fractals/torrent/TorrentIO.cpp
            fractals/persist/storage.cpp
            fractals/persist/data.cpp fractals/torrent/File.cpp)

# Add source to this project's executable.
add_library(Fractals_lib ${SOURCES})

target_include_directories(Fractals_lib PUBLIC ${NEITHER_INCLUDE_DIR})
target_link_libraries(Fractals_lib bencode)
target_link_libraries(Fractals_lib OpenSSL::SSL)
target_link_libraries(Fractals_lib ftxui::screen ftxui::dom ftxui::component)
target_link_libraries(Fractals_lib ${CURL_LIBRARIES})
target_link_libraries(Fractals_lib ${BOOST_LIBRARIES})
target_link_libraries(Fractals_lib ${EPOLL_WRAPPER_LIBRARIES})
target_link_libraries(Fractals_lib sqlite3)
target_link_libraries(Fractals_lib Boost::log boost_log_setup)
target_link_libraries(Fractals_lib pthread dl)
target_link_libraries(Fractals_lib m)

add_executable (Fractals main.cpp)
target_link_libraries(Fractals Fractals_lib)

set_target_properties(Fractals PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            # CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path}
            )

# set_target_properties(Fractals_lib PROPERTIES
#             CXX_STANDARD 17
#             CXX_EXTENSIONS OFF
#             )