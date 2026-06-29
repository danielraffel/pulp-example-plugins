# MPE Spreader — give every held note its own MPE member channel. See README.md.

if(PULP_BUILD_TESTS)
    add_executable(mpe-spreader-test test_mpe_spreader.cpp)
    target_link_libraries(mpe-spreader-test PRIVATE pulp::format pulp::view Catch2::Catch2WithMain)
    target_include_directories(mpe-spreader-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(mpe-spreader-test)
endif()

# A pure MIDI effect — CLAP carries per-channel MIDI cleanly.
pulp_add_plugin(MpeSpreader
    FORMATS           CLAP Standalone
    PLUGIN_NAME       "MpeSpreader"
    BUNDLE_ID         "com.pulp.examples.mpe-spreader"
    MANUFACTURER      "Pulp Examples"
    VERSION           "0.1.0"
    CATEGORY          MidiEffect
    PLUGIN_CODE       "PMpe"
    MANUFACTURER_CODE "PlpX"
)
