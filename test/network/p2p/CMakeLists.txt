add_executable(
    testReceiveClient
    testReceiveClient.cpp
)

target_link_libraries(testReceiveClient gtest_main gmock_main Fractals_lib)

add_executable(
    testWorkQueue
    testWorkQueue.cpp
)

target_link_libraries(testWorkQueue gtest_main gmock_main Fractals_lib)

include(GoogleTest)
gtest_discover_tests(testReceiveClient)
gtest_discover_tests(testWorkQueue)

set_tests_properties(${Tests} PROPERTIES TIMEOUT 1)
add_compile_options(-fsanitize=leak,address,undefined -fno-omit-frame-pointer -fno-common -O1)
link_libraries(-fsanitize=leak,address,undefined)