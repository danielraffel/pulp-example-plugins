# Synth With Presets — instrument with a factory preset bank. See README.md.

if(PULP_BUILD_TESTS)
    add_executable(synth-with-presets-test test_synth_with_presets.cpp)
    target_link_libraries(synth-with-presets-test PRIVATE pulp::format pulp::signal Catch2::Catch2WithMain)
    target_include_directories(synth-with-presets-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(synth-with-presets-test)
endif()

pulp_add_plugin(SynthWithPresets
    FORMATS           CLAP Standalone
    PLUGIN_NAME       "SynthPresets"
    BUNDLE_ID         "com.pulp.examples.synth-with-presets"
    MANUFACTURER      "Pulp Examples"
    VERSION           "0.1.0"
    CATEGORY          Instrument
    PLUGIN_CODE       "PSwp"
    MANUFACTURER_CODE "PlpX"
)
