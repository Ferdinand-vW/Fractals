cmake_minimum_required (VERSION 3.8)
project(Fractals)

set(SOURCE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
set(BENCODE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/bencode/include)
set(NEITHER_HEADER_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/neither/neither/include)
set(NEITHER_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/neither/neither/build/include)

# scaffold neither headers
set(NEITHER_HEADERS "either.hpp" "lift.hpp" "maybe.hpp" "neither.hpp" "traits.hpp" "try.hpp")
foreach (H ${NEITHER_HEADERS}) 
    file(COPY ${NEITHER_HEADER_DIR}/${H} DESTINATION ${NEITHER_INCLUDE_DIR})
endforeach()

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include_directories(include)
include_directories(${NEITHER_INCLUDE_DIR})


add_subdirectory(libs/bencode)
set_target_properties(bencode_run PROPERTIES EXCLUDE_FROM_ALL True)

add_subdirectory(src)