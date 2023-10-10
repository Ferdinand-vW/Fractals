add_executable(
    testDiskIOService
    testDiskIOService.cpp
)

target_link_libraries(testDiskIOService gtest_main gmock_main Fractals_lib)

include(GoogleTest)
gtest_discover_tests(testDiskIOService)

set_tests_properties(${Tests} PROPERTIES TIMEOUT 1)
add_compile_options(-fsanitize=leak,address,undefined -fno-omit-frame-pointer -fno-common -O1)
link_libraries(-fsanitize=leak,address,undefined)