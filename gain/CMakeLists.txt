# Gain — a plain utility effect (linear gain + equal-power pan). See README.md.

if(PULP_BUILD_TESTS)
    add_executable(gain-test test_gain.cpp)
    target_link_libraries(gain-test PRIVATE pulp::format pulp::view Catch2::Catch2WithMain)
    target_include_directories(gain-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(gain-test)
endif()

pulp_add_plugin(Gain
    FORMATS           VST3 CLAP Standalone
    PLUGIN_NAME       "Gain"
    BUNDLE_ID         "com.pulp.examples.gain"
    MANUFACTURER      "Pulp Examples"
    VERSION           "0.1.0"
    CATEGORY          Effect
    PLUGIN_CODE       "PGn2"
    MANUFACTURER_CODE "PlpX"
)
