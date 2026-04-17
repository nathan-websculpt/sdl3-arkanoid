find_package(GTest CONFIG REQUIRED)

add_executable(arkanoid_tests
    test_state_machine.cpp
)

target_link_libraries(arkanoid_tests
    PRIVATE
        arkanoid::lib
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(arkanoid_tests PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(arkanoid_tests PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(arkanoid_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GoogleTest)
gtest_discover_tests(arkanoid_tests)