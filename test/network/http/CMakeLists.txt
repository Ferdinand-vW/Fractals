add_executable(
    testTracker
    testTracker.cpp
)

target_link_libraries(testTracker gtest_main gmock_main Fractals_lib)

include(GoogleTest)
gtest_discover_tests(testTracker)