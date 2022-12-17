add_executable(
    testConnectionReadHandler
    testConnectionReadHandler.cpp
)

target_link_libraries(testConnectionReadHandler gtest_main gmock_main Fractals_lib)

add_executable(
    testConnectionWriteHandler
    testConnectionWriteHandler.cpp
)

target_link_libraries(testConnectionWriteHandler gtest_main gmock_main Fractals_lib)

add_executable(
    testWorkQueue
    testWorkQueue.cpp
)

target_link_libraries(testWorkQueue gtest_main gmock_main Fractals_lib)

add_executable(
    testBufferManager
    testBufferManager.cpp
)

target_link_libraries(testBufferManager gtest_main gmock_main Fractals_lib)


add_executable(
    testBitTorrentEncoder
    testBitTorrentEncoder.cpp
)

target_link_libraries(testBitTorrentEncoder gtest_main gmock_main Fractals_lib)


include(GoogleTest)
gtest_discover_tests(testConnectionReadHandler)
gtest_discover_tests(testConnectionWriteHandler)
gtest_discover_tests(testWorkQueue)

set_tests_properties(${Tests} PROPERTIES TIMEOUT 1)
add_compile_options(-fsanitize=leak,address,undefined -fno-omit-frame-pointer -fno-common -O1)
link_libraries(-fsanitize=leak,address,undefined)