cmake_minimum_required(VERSION 3.25)
project(demo-game VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Debug if no build type is specified (single-config generators like MinGW Makefiles)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# Find SDL3 (expects SDL3_DIR or CMAKE_PREFIX_PATH to point to the SDL3 install)
find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)

add_executable(demo-game
    src/main.cpp
)

target_link_libraries(demo-game PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

# Per-configuration compiler flags
target_compile_options(demo-game PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
    $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
)

# Place the executable in a config-named subfolder (e.g. build/Debug/, build/Release/)
set_target_properties(demo-game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}"
)

# Copy SDL3 shared library next to the executable on Windows
if(WIN32)
    add_custom_command(TARGET demo-game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:demo-game>
        COMMENT "Copying SDL3.dll to output directory"
    )
    add_custom_command(TARGET demo-game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3_image::SDL3_image>
            $<TARGET_FILE_DIR:demo-game>
        COMMENT "Copying SDL3_image.dll to output directory"
    )
endif()
