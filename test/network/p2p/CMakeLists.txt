# add_executable(
#     test
#     testReceiver.cpp
# )


# target_link_libraries(testReceiver gtest_main gmock_main Fractals_lib)

add_executable(
    testEpoll
    testEpoll.cpp
)


target_link_libraries(testEpoll gtest_main gmock_main Fractals_lib)

include(GoogleTest)
# gtest_discover_tests(testReceiver)
gtest_discover_tests(testEpoll)