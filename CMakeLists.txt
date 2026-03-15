cmake_minimum_required(VERSION 3.20)

project(doom_like_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib QUIET)

if(NOT raylib_FOUND)
    include(FetchContent)

    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(raylib)
endif()

add_executable(doom_like
    src/main.cpp
    src/core/audio.cpp
    src/core/game.cpp
    src/entities/player.cpp
    src/entities/target.cpp
    src/render/renderer.cpp
    src/world/world.cpp
)

target_include_directories(doom_like PRIVATE include)
target_link_libraries(doom_like PRIVATE raylib)

if(MSVC)
    target_compile_options(doom_like PRIVATE /W4 /permissive-)
else()
    target_compile_options(doom_like PRIVATE -Wall -Wextra -Wpedantic)
endif()
