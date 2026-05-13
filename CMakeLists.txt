cmake_minimum_required(VERSION 3.21)

project(arkanoid_portfolio LANGUAGES CXX)

if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

option(ARKANOID_ENABLE_MSVC_ANALYZE "Enable MSVC /analyze for selected targets" OFF)

include(CTest)
include(GNUInstallDirs)

function(arkanoid_apply_project_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /EHsc)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(arkanoid_lib STATIC)
add_library(arkanoid::lib ALIAS arkanoid_lib)

set(ARKANOID_LIB_SOURCES
    src/core/game.cpp
)

set(ARKANOID_APP_SOURCES
    src/main.cpp
    src/app/application.cpp
    src/app/frame_timing.cpp
    src/app/input.cpp
    src/app/run_mode.cpp
    src/render/render_frame.cpp
)

target_sources(arkanoid_lib PRIVATE ${ARKANOID_LIB_SOURCES})

target_compile_features(arkanoid_lib PUBLIC cxx_std_20)

target_include_directories(arkanoid_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

find_package(SDL3 CONFIG REQUIRED)

arkanoid_apply_project_warnings(arkanoid_lib)

add_executable(arkanoid)
target_sources(arkanoid PRIVATE ${ARKANOID_APP_SOURCES})
target_include_directories(arkanoid
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(arkanoid
    PRIVATE
        arkanoid::lib
        SDL3::SDL3
)
arkanoid_apply_project_warnings(arkanoid)

if(MSVC)
    target_link_options(arkanoid PRIVATE
        "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>"
        "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>"
    )
endif()

if(MSVC AND ARKANOID_ENABLE_MSVC_ANALYZE)
    target_compile_options(arkanoid_lib PRIVATE /analyze)
    target_compile_options(arkanoid PRIVATE /analyze)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(TARGETS arkanoid
    RUNTIME_DEPENDENCY_SET arkanoid_runtime_dependencies
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_LICENSES.txt"
    DESTINATION "."
)

if(WIN32)
    install(RUNTIME_DEPENDENCY_SET arkanoid_runtime_dependencies
        PRE_EXCLUDE_REGEXES
            [[api-ms-.*]]
            [[ext-ms-.*]]
        POST_EXCLUDE_REGEXES
            [[.*[\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\/][Ss][Yy][Ss][Tt][Ee][Mm]32[\\/].*]]
            [[.*[\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss][\\/][Ss][Yy][Ss][Ww][Oo][Ww]64[\\/].*]]
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
    include(InstallRequiredSystemLibraries)
endif()
