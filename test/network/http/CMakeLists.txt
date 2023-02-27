add_executable(
    testTrackerRequest
    testTrackerRequest.cpp
)

target_link_libraries(testTrackerRequest gtest_main gmock_main Fractals_lib)

add_executable(
    testAnnounceService
    testAnnounceService.cpp
)

target_link_libraries(testAnnounceService gtest_main gmock_main Fractals_lib)


include(GoogleTest)
gtest_discover_tests(testTrackerRequest)
gtest_discover_tests(testAnnounceService)

set_tests_properties(${Tests} PROPERTIES TIMEOUT 1)
add_compile_options(-fsanitize=leak,address,undefined -fno-omit-frame-pointer -fno-common -O1)
link_libraries(-fsanitize=leak,address,undefined)