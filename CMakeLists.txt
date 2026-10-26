cmake_minimum_required(VERSION 3.24)   # 4.3 only works with very new CMake; 3.24 is enough for FetchContent
project(torre_de_hanoi CXX C)          # raylib is written in C, so we enable both languages

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent downloads a dependency when CMake configures the project.
# This means nobody has to install raylib by hand, including your professor.
include(FetchContent)

FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz)   # one release archive, not the whole git history
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # skip building raylib's ~100 examples
FetchContent_MakeAvailable(raylib)     # makes a target called "raylib" we can link against

FetchContent_Declare(raygui
        URL https://github.com/raysan5/raygui/archive/refs/tags/4.0.tar.gz)
FetchContent_MakeAvailable(raygui)   # raygui is a single header, so we only need its include path

# Collect every .cpp under src/. Note: after adding a NEW file, reload CMake in CLion.
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS src/*.cpp)
add_executable(torre_de_hanoi ${SRC})

# Lets you write #include "core/Move.h" from anywhere in src/
target_include_directories(torre_de_hanoi PRIVATE src)
target_include_directories(torre_de_hanoi SYSTEM PRIVATE ${raygui_SOURCE_DIR}/src)   # SYSTEM: hide warnings from raygui's own code
target_link_libraries(torre_de_hanoi PRIVATE raylib)

# Copy assets/ (fonts) next to the executable after every build, so the program finds them
add_custom_command(TARGET torre_de_hanoi POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:torre_de_hanoi>/assets)


if(APPLE)   # on macOS, raylib needs these system frameworks for windows, input and OpenGL
    target_link_libraries(torre_de_hanoi PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()