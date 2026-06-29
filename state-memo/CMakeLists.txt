# State Memo — custom plugin state beyond parameters. See README.md.

if(PULP_BUILD_TESTS)
    add_executable(state-memo-test test_state_memo.cpp)
    target_link_libraries(state-memo-test PRIVATE pulp::format Catch2::Catch2WithMain)
    target_include_directories(state-memo-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(state-memo-test)
endif()

pulp_add_plugin(StateMemo
    FORMATS           VST3 CLAP Standalone
    PLUGIN_NAME       "StateMemo"
    BUNDLE_ID         "com.pulp.examples.state-memo"
    MANUFACTURER      "Pulp Examples"
    VERSION           "0.1.0"
    CATEGORY          Effect
    PLUGIN_CODE       "PSme"
    MANUFACTURER_CODE "PlpX"
)
