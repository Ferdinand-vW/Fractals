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

include(GoogleTest)
gtest_discover_tests(testEncode)
gtest_discover_tests(testMaybe)
gtest_discover_tests(testUtils)