add_executable(
    testPersistService
    testPersistService.cpp
)

target_link_libraries(testPersistService gtest_main gmock_main Fractals_lib)

include(GoogleTest)
gtest_discover_tests(testPersistService)

set_tests_properties(${Tests} PROPERTIES TIMEOUT 1)
add_compile_options(-fsanitize=leak,address,undefined -fno-omit-frame-pointer -fno-common -O1)
link_libraries(-fsanitize=leak,address,undefined)