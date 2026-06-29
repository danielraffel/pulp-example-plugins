# MIDI Inspector — pass-through that logs events for display. See README.md.

if(PULP_BUILD_TESTS)
    add_executable(midi-inspector-test test_midi_inspector.cpp)
    target_link_libraries(midi-inspector-test PRIVATE pulp::format pulp::view Catch2::Catch2WithMain)
    target_include_directories(midi-inspector-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(midi-inspector-test)
endif()

# A pure MIDI effect — CLAP carries MIDI cleanly; standalone for quick checks.
pulp_add_plugin(MidiInspector
    FORMATS           CLAP Standalone
    PLUGIN_NAME       "MidiInspector"
    BUNDLE_ID         "com.pulp.examples.midi-inspector"
    MANUFACTURER      "Pulp Examples"
    VERSION           "0.1.0"
    CATEGORY          MidiEffect
    PLUGIN_CODE       "PMin"
    MANUFACTURER_CODE "PlpX"
)
