cmake_minimum_required(VERSION 3.21)

project(arkanoid_portfolio LANGUAGES CXX)

option(ARKANOID_ENABLE_MSVC_ANALYZE "Enable MSVC /analyze for selected targets" OFF)

include(CTest)

add_library(arkanoid_lib STATIC)
add_library(arkanoid::lib ALIAS arkanoid_lib)

set(ARKANOID_LIB_SOURCES
    src/core/game.cpp
)

set(ARKANOID_APP_SOURCES
    src/main.cpp
)

target_sources(arkanoid_lib PRIVATE ${ARKANOID_LIB_SOURCES})

target_compile_features(arkanoid_lib PUBLIC cxx_std_20)

target_include_directories(arkanoid_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(arkanoid_lib PUBLIC SDL3::SDL3)

if(MSVC)
    target_compile_options(arkanoid_lib PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(arkanoid_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(arkanoid)
target_sources(arkanoid PRIVATE ${ARKANOID_APP_SOURCES})
target_link_libraries(arkanoid PRIVATE arkanoid::lib)

if(MSVC AND ARKANOID_ENABLE_MSVC_ANALYZE)
    target_compile_options(arkanoid_lib PRIVATE /analyze)
    target_compile_options(arkanoid PRIVATE /analyze)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
