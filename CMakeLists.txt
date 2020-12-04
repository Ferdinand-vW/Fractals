cmake_minimum_required (VERSION 3.8)
project(Fractals)

set(BENCODE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/deps/bencode/include)

# include_directories(deps/bencode/include)
add_subdirectory(libs/bencode)
add_subdirectory(src)