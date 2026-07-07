cmake_minimum_required(VERSION 3.24)
project(pulp-example-plugins LANGUAGES CXX)

# Consume the Pulp SDK. The simplest path is to scaffold against an existing
# Pulp checkout/install with `pulp create`, which pins the SDK for you. To wire
# this repo directly, point CMake at a Pulp build or install tree, e.g.:
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_PREFIX_PATH=/path/to/pulp/install
#
# find_package(Pulp) provides pulp::format, pulp::signal, and the
# pulp_add_plugin() helper. The repo fetches its own Catch2 when tests are
# enabled so it works against an installed SDK.
find_package(Pulp CONFIG REQUIRED)

option(PULP_BUILD_TESTS "Build the example tests" ON)
if(PULP_BUILD_TESTS)
    include(FetchContent)
    if(NOT TARGET Catch2::Catch2WithMain)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        v3.7.1
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(Catch2)
    endif()
    if(DEFINED catch2_SOURCE_DIR AND EXISTS "${catch2_SOURCE_DIR}/extras/Catch.cmake")
        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    endif()
    include(Catch)
    enable_testing()
endif()

add_subdirectory(midi-transpose)
add_subdirectory(sysex-echo)
add_subdirectory(midi-inspector)
add_subdirectory(state-memo)
add_subdirectory(mpe-spreader)
add_subdirectory(mono-synth)
add_subdirectory(synth-with-presets)
add_subdirectory(gain)
add_subdirectory(gui-zoo)

# Combined render check for every plugin's dark Ink & Signal editor. Needs
# pulp::view (Ink & Signal theme + Skia screenshot); skips at runtime when the
# Skia raster backend isn't compiled into the SDK build.
if(PULP_BUILD_TESTS AND TARGET pulp::view)
    add_executable(editors-test test_editors.cpp)
    target_link_libraries(editors-test
        PRIVATE pulp::format pulp::signal pulp::view Catch2::Catch2WithMain)
    target_include_directories(editors-test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/midi-transpose
        ${CMAKE_CURRENT_SOURCE_DIR}/sysex-echo
        ${CMAKE_CURRENT_SOURCE_DIR}/midi-inspector
        ${CMAKE_CURRENT_SOURCE_DIR}/mpe-spreader
        ${CMAKE_CURRENT_SOURCE_DIR}/state-memo
        ${CMAKE_CURRENT_SOURCE_DIR}/mono-synth
        ${CMAKE_CURRENT_SOURCE_DIR}/synth-with-presets
        ${CMAKE_CURRENT_SOURCE_DIR}/gain)
    target_compile_definitions(editors-test PRIVATE
        SCREENSHOTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/screenshots")
    catch_discover_tests(editors-test)
endif()
