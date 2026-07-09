# WASM build for the pulp-example-plugins web demos.
#
# This is a STANDALONE emcmake project — it is deliberately NOT included by the
# repo root CMakeLists.txt. The root build is the NATIVE plugin build (Skia
# editors, VST3/AU/etc.); this one cross-compiles the same plugin headers to
# headless WebAssembly WAMv2 (Web Audio Modules v2) DSP modules for GitHub Pages.
#
# Build (see ../scripts/build-web.sh for the packaging step CI runs):
#   emcmake cmake -S web -B web/build -DCMAKE_BUILD_TYPE=Release \
#       -DPULP_ROOT=<pulp source checkout> \
#       -DPULP_WAM_CHOC_INCLUDE=<dir containing choc/>
#   cmake --build web/build
#
# Output: web/build/<Name>.js — a SINGLE_FILE ES-module DSP factory per plugin
# (the wasm is BASE64-embedded, so there is no separate .wasm fetch; that is what
# lets it run inside an AudioWorkletGlobalScope, which has no fetch/async
# compile). No cross-origin isolation is needed: the build is single-threaded,
# so no SharedArrayBuffer, so no COOP/COEP — which is why it works on stock
# GitHub Pages, whose headers cannot be customised.

cmake_minimum_required(VERSION 3.20)
project(pulp-example-web-demos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PULP_ROOT must point at a pulp SOURCE checkout: PulpWam.cmake compiles a
# curated DSP subset straight out of ${PULP_ROOT}/core/*/src, so an installed
# SDK is NOT enough. choc headers come from -DPULP_WAM_CHOC_INCLUDE=<dir
# containing choc/> (handled inside PulpWam.cmake).
if(NOT PULP_ROOT)
    message(FATAL_ERROR
        "pulp-example-web-demos: pass -DPULP_ROOT=<pulp source checkout>. "
        "PulpWam.cmake builds the headless DSP subset from ${PULP_ROOT}/core/*/src.")
endif()
if(NOT EXISTS "${PULP_ROOT}/tools/cmake/PulpWam.cmake")
    message(FATAL_ERROR
        "pulp-example-web-demos: ${PULP_ROOT}/tools/cmake/PulpWam.cmake not found — "
        "is -DPULP_ROOT a real pulp source tree?")
endif()
include(${PULP_ROOT}/tools/cmake/PulpWam.cmake)

# The plugin headers guard their native editor bodies with `#if !PULP_HEADLESS`
# (create_view() etc. pull in the Skia/view stack that does not exist in wasm).
# pulp_add_wam_plugin does not expose target_compile_definitions, so define the
# guard macro globally before declaring any target.
add_compile_definitions(PULP_HEADLESS=1)

# Each demoable plugin -> one SINGLE_FILE WAM DSP module. INCLUDES points at the
# plugin's source dir (one level up) so its header resolves.
#
# gui-zoo is DELIBERATELY EXCLUDED: it is a Processor whose only content is
# create_view(), and create_view() hard-returns nullptr in every WASM build
# (core/format/src/wasm/headless_defaults.cpp). With no DSP and no reachable UI
# on this path it is structurally undemoable, so there is nothing to ship.
pulp_add_wam_plugin(MonoSynth        SINGLE_FILE ENTRY ${CMAKE_SOURCE_DIR}/mono_synth_wasm.cpp         INCLUDES ${CMAKE_SOURCE_DIR}/../mono-synth)
pulp_add_wam_plugin(SynthWithPresets SINGLE_FILE ENTRY ${CMAKE_SOURCE_DIR}/synth_with_presets_wasm.cpp INCLUDES ${CMAKE_SOURCE_DIR}/../synth-with-presets)
pulp_add_wam_plugin(Gain             SINGLE_FILE ENTRY ${CMAKE_SOURCE_DIR}/gain_wasm.cpp               INCLUDES ${CMAKE_SOURCE_DIR}/../gain)
pulp_add_wam_plugin(StateMemo        SINGLE_FILE ENTRY ${CMAKE_SOURCE_DIR}/state_memo_wasm.cpp         INCLUDES ${CMAKE_SOURCE_DIR}/../state-memo)
pulp_add_wam_plugin(MidiTranspose    SINGLE_FILE ENTRY ${CMAKE_SOURCE_DIR}/midi_transpose_wasm.cpp     INCLUDES ${CMAKE_SOURCE_DIR}/../midi-transpose)
pulp_add_wam_plugin(MpeSpreader      SINGLE_FILE ENTRY ${CMAKE_SOURCE_DIR}/mpe_spreader_wasm.cpp       INCLUDES ${CMAKE_SOURCE_DIR}/../mpe-spreader)
pulp_add_wam_plugin(MidiInspector    SINGLE_FILE ENTRY ${CMAKE_SOURCE_DIR}/midi_inspector_wasm.cpp     INCLUDES ${CMAKE_SOURCE_DIR}/../midi-inspector)
pulp_add_wam_plugin(SysexEcho        SINGLE_FILE ENTRY ${CMAKE_SOURCE_DIR}/sysex_echo_wasm.cpp         INCLUDES ${CMAKE_SOURCE_DIR}/../sysex-echo)
