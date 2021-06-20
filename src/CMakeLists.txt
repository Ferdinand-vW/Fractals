cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

# file(GLOB SOURCES "*.cpp" "")
set(SOURCES main.cpp 
            app/Client.cpp
            app/Feedback.cpp
            app/TorrentController.cpp
            app/TerminalInput.cpp
            app/TorrentDisplay.cpp
            app/TorrentView.cpp
            common/encode.cpp 
            common/utils.cpp
            network/http/Tracker.cpp
            network/http/Peer.cpp
            network/p2p/Connection.cpp
            network/p2p/BitTorrent.cpp
            network/p2p/Client.cpp 
            network/p2p/Message.cpp 
            network/p2p/MessageType.cpp
            network/http/Peer.cpp
            torrent/PieceData.cpp
            torrent/Torrent.cpp
            persist/storage.cpp
            persist/data.cpp)

# Add source to this project's executable.
add_executable (Fractals ${SOURCES})
# add_library(Fractals_lib STATIC ${SOURCES})

target_include_directories(Fractals PUBLIC ${BENCODE_INCLUDE_DIR})
target_include_directories(Fractals PUBLIC ${NEITHER_INCLUDE_DIR})
target_include_directories(Fractals PUBLIC ${TERMINALPP_INCLUDE_DIR})
target_link_libraries(Fractals bencode_lib)
target_link_libraries(Fractals OpenSSL::SSL)
target_link_libraries(Fractals ftxui::screen ftxui::dom ftxui::component)
target_link_libraries(Fractals ${CURL_LIBRARIES})
target_link_libraries(Fractals ${BOOST_LIBRARIES})
target_link_libraries(Fractals sqlite3)
target_link_libraries(Fractals Boost::log boost_log_setup)
target_link_libraries(Fractals pthread dl)

set_target_properties(Fractals PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            )

# set_target_properties(Fractals_lib PROPERTIES
#             CXX_STANDARD 17
#             CXX_EXTENSIONS OFF
#             )