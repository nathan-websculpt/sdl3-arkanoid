find_package(GTest CONFIG REQUIRED)

add_executable(arkanoid_tests
    test_countdown_and_serve.cpp
    test_dt_validation.cpp
    test_wall_and_paddle_collision.cpp
    test_bricks_and_board_clear.cpp
    test_life_loss_and_reset.cpp
)

target_link_libraries(arkanoid_tests
    PRIVATE
        arkanoid::lib
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(arkanoid_tests PRIVATE cxx_std_20)

arkanoid_apply_project_warnings(arkanoid_tests)

include(GoogleTest)
gtest_discover_tests(arkanoid_tests)
