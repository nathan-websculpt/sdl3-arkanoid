find_package(GTest CONFIG REQUIRED)

set(ARKANOID_TEST_CORE_SOURCES)
foreach(source_path IN LISTS ARKANOID_LIB_SOURCES)
    list(APPEND ARKANOID_TEST_CORE_SOURCES "${PROJECT_SOURCE_DIR}/${source_path}")
endforeach()

add_library(arkanoid_test_core STATIC ${ARKANOID_TEST_CORE_SOURCES})

target_compile_features(arkanoid_test_core PUBLIC cxx_std_20)

target_include_directories(arkanoid_test_core
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_compile_definitions(arkanoid_test_core
    PUBLIC
        ARKANOID_ENABLE_GAME_TEST_ACCESS
)

arkanoid_apply_project_warnings(arkanoid_test_core)

add_executable(arkanoid_tests
    test_countdown_and_serve.cpp
    test_dt_validation.cpp
    test_wall_and_paddle_collision.cpp
    test_bricks_and_board_clear.cpp
    test_life_loss_and_reset.cpp
)

target_link_libraries(arkanoid_tests
    PRIVATE
        arkanoid_test_core
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(arkanoid_tests PRIVATE cxx_std_20)

arkanoid_apply_project_warnings(arkanoid_tests)

include(GoogleTest)
gtest_discover_tests(arkanoid_tests)
