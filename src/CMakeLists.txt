cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

# file(GLOB SOURCES "*.cpp" "")
set(SOURCES main.cpp 
            fractals/app/Client.cpp
            fractals/app/Feedback.cpp
            fractals/app/ScreenTicker.cpp
            fractals/app/TorrentController.cpp
            fractals/app/TerminalInput.cpp
            fractals/app/TorrentDisplay.cpp
            fractals/app/TorrentView.cpp
            fractals/common/encode.cpp
            fractals/common/utils.cpp
            fractals/network/http/Tracker.cpp
            fractals/network/http/Peer.cpp
            fractals/network/p2p/Connection.cpp
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
add_executable (Fractals ${SOURCES})
# add_library(Fractals_lib STATIC ${SOURCES})

#target_include_directories(Fractals PUBLIC ${BENCODE_INCLUDE_DIR})
target_include_directories(Fractals PUBLIC ${NEITHER_INCLUDE_DIR})
target_link_libraries(Fractals bencode::bencode)
target_link_libraries(Fractals OpenSSL::SSL)
target_link_libraries(Fractals ftxui::screen ftxui::dom ftxui::component)
target_link_libraries(Fractals ${CURL_LIBRARIES})
target_link_libraries(Fractals ${BOOST_LIBRARIES})
target_link_libraries(Fractals sqlite3)
target_link_libraries(Fractals Boost::log boost_log_setup)
target_link_libraries(Fractals pthread dl)
target_link_libraries(Fractals m)

set_target_properties(Fractals PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            # CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path}
            )

# set_target_properties(Fractals_lib PROPERTIES
#             CXX_STANDARD 17
#             CXX_EXTENSIONS OFF
#             )