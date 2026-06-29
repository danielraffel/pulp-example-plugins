cmake_minimum_required(VERSION 3.24)
project(pulp-example-plugins LANGUAGES CXX)

# Consume the Pulp SDK. The simplest path is to scaffold against an existing
# Pulp checkout/install with `pulp create`, which pins the SDK for you. To wire
# this repo directly, point CMake at a Pulp build or install tree, e.g.:
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_PREFIX_PATH=/path/to/pulp/install
#
# find_package(Pulp) provides pulp::format, pulp::signal, the pulp_add_plugin()
# helper, and a bundled Catch2 for the tests.
find_package(Pulp CONFIG REQUIRED)

option(PULP_BUILD_TESTS "Build the example tests" ON)
enable_testing()

add_subdirectory(midi-transpose)
add_subdirectory(sysex-echo)
add_subdirectory(midi-inspector)
add_subdirectory(state-memo)
add_subdirectory(mpe-spreader)
add_subdirectory(mono-synth)
add_subdirectory(synth-with-presets)
add_subdirectory(gui-zoo)
