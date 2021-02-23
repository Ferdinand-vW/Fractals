cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

# file(GLOB SOURCES "*.cpp" "")
set(SOURCES main.cpp network/TrackerRequest.cpp common/encode.cpp common/utils.cpp)

# Add source to this project's executable.
add_executable (Fractals ${SOURCES})
# add_library(Fractals_lib STATIC ${SOURCES})

target_include_directories(Fractals PUBLIC ${BENCODE_INCLUDE_DIR})
target_include_directories(Fractals PUBLIC ${NEITHER_INCLUDE_DIR})
target_link_libraries(Fractals bencode_lib)
target_link_libraries(Fractals OpenSSL::SSL)
target_link_libraries(Fractals ${CURL_LIBRARIES})
# target_link_libraries(Fractals_lib bencode_lib)

set_target_properties(Fractals PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            )

# set_target_properties(Fractals_lib PROPERTIES
#             CXX_STANDARD 17
#             CXX_EXTENSIONS OFF
#             )