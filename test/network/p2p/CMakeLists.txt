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
    testBufferedQueueManager
    testBufferedQueueManager.cpp
)

target_link_libraries(testBufferedQueueManager gtest_main gmock_main Fractals_lib)

add_executable(
    testBitTorrentManager
    testBitTorrentManager.cpp
)

target_link_libraries(testBitTorrentManager gtest_main gmock_main Fractals_lib)

add_executable(
    testBitTorrentEncoder
    testBitTorrentEncoder.cpp
)

target_link_libraries(testBitTorrentEncoder gtest_main gmock_main Fractals_lib)


add_executable(
    testPieceStateManager
    testPieceStateManager.cpp
)

target_link_libraries(testPieceStateManager gtest_main gmock_main Fractals_lib)

add_executable(
    testProtocol
    testProtocol.cpp
)

target_link_libraries(testProtocol gtest_main gmock_main Fractals_lib)


include(GoogleTest)
gtest_discover_tests(testConnectionReadHandler)
gtest_discover_tests(testConnectionWriteHandler)
gtest_discover_tests(testWorkQueue)
gtest_discover_tests(testBufferedQueueManager)
gtest_discover_tests(testBitTorrentEncoder)
gtest_discover_tests(testPieceStateManager)
gtest_discover_tests(testProtocol)

set_tests_properties(${Tests} PROPERTIES TIMEOUT 1)
add_compile_options(-fsanitize=leak,address,undefined -fno-omit-frame-pointer -fno-common -O1)
link_libraries(-fsanitize=leak,address,undefined)