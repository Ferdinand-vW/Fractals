add_executable(
    testReceiver
    testReceiver.cpp
)

target_link_libraries(testReceiver gtest_main gmock_main Fractals_lib)

add_executable(
    testWorkQueue
    testWorkQueue.cpp
)

target_link_libraries(testWorkQueue gtest_main gmock_main Fractals_lib)

include(GoogleTest)
gtest_discover_tests(testReceiver)
gtest_discover_tests(testWorkQueue)

set_tests_properties(${Tests} PROPERTIES TIMEOUT 1)