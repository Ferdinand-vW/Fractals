add_executable(
    testCurlPoll
    testCurlPoll.cpp
)
target_link_libraries(testCurlPoll gtest_main gmock_main Fractals_lib)

add_executable(
    testEncode
    testEncode.cpp
)
target_link_libraries(testEncode gtest_main gmock_main Fractals_lib)

add_executable(
    testMaybe
    testMaybe.cpp
)
target_link_libraries(testMaybe gtest_main gtest gmock gmock_main Fractals_lib)

add_executable(
    testUtils
    testUtils.cpp
)
target_link_libraries(testUtils gtest_main gmock_main Fractals_lib)

add_executable(
    testWorkQueue
    testWorkQueue.cpp
)

target_link_libraries(testWorkQueue gtest_main gmock_main Fractals_lib)


include(GoogleTest)
gtest_discover_tests(testCurlPoll)
gtest_discover_tests(testEncode)
gtest_discover_tests(testMaybe)
gtest_discover_tests(testUtils)
gtest_discover_tests(testWorkQueue)