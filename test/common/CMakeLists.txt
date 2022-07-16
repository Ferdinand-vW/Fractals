add_executable(
    testEncode
    testEncode.cpp
)

target_link_libraries(testEncode GTest::gtest_main Fractals_lib)

include(GoogleTest)
gtest_discover_tests(testEncode)