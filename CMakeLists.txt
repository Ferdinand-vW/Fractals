cmake_minimum_required (VERSION 3.8)
project(Fractals)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
set(BENCODE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/bencode/include)
set(SQLITE_ORM_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/sqlite_orm/include)
set(NEITHER_HEADER_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/neither/neither/include)
set(NEITHER_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/neither/neither/build/include)
set(FTXUI_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/FTXUI/include)
set(BENCODE_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/libs/bencode/include)

# scaffold neither headers
set(NEITHER_HEADERS "either.hpp" "lift.hpp" "maybe.hpp" "neither.hpp" "traits.hpp" "try.hpp")
foreach (H ${NEITHER_HEADERS}) 
    file(COPY ${NEITHER_HEADER_DIR}/${H} DESTINATION ${NEITHER_INCLUDE_DIR})
endforeach()

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
find_package(Boost 1.76.0 COMPONENTS system thread log 
                            stacktrace_basic 
                            stacktrace_backtrace 
                            stacktrace_addr2line 
                            stacktrace_noop REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

find_package(SQLite3)
include_directories(${SQLite3_INCLUDE_DIR})

include_directories(include)
include_directories(SYSTEM ${NEITHER_INCLUDE_DIR})

include_directories(SYSTEM ${SQLITE_ORM_INCLUDE_DIR})

find_package(ftxui)
include_directories(${FTXUI_INCLUDE_DIR})

find_package(bencode REQUIRED)
include_directories(${BENCODE_INCLUDE_DIRS})

find_package(epoll_wrapper REQUIRED)
include_directories(${EPOLL_WRAPPER_INCLUDE_DIRS})

find_package(spdlog REQUIRED)
include_directories(${SPDLOG_INCLUDE_DIRS})

add_subdirectory(src)

enable_testing ()
add_subdirectory(test)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
