# SysEx Echo — round-trips System Exclusive messages. See README.md.

if(PULP_BUILD_TESTS)
    add_executable(sysex-echo-test test_sysex_echo.cpp)
    target_link_libraries(sysex-echo-test PRIVATE pulp::format Catch2::Catch2WithMain)
    target_include_directories(sysex-echo-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(sysex-echo-test)
endif()

# A pure MIDI effect — CLAP carries SysEx cleanly; standalone for quick checks.
pulp_add_plugin(SysexEcho
    FORMATS           CLAP Standalone
    PLUGIN_NAME       "SysExEcho"
    BUNDLE_ID         "com.pulp.examples.sysex-echo"
    MANUFACTURER      "Pulp Examples"
    VERSION           "0.1.0"
    CATEGORY          MidiEffect
    PLUGIN_CODE       "PSyx"
    MANUFACTURER_CODE "PlpX"
)
