# gui-zoo — a UI-fixture PLUGIN: no DSP, no params, its editor is a scrolling
# board of Pulp's widget primitives under the dark Ink & Signal theme, so the
# widget set can be browsed live in any AU / VST3 / CLAP host.
#
# It also keeps a headless screenshot fixture: build_gui_zoo() renders the full,
# non-scrolling board and the test compares it against baseline.png. The test
# only runs where the view + Skia raster backend are available; it SKIPs cleanly
# otherwise (e.g. a build without Skia).

if(PULP_BUILD_TESTS)
    add_executable(gui-zoo-test test_gui_zoo.cpp)
    target_link_libraries(gui-zoo-test PRIVATE pulp::view Catch2::Catch2WithMain)
    target_include_directories(gui-zoo-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(gui-zoo-test PRIVATE GUI_ZOO_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    catch_discover_tests(gui-zoo-test)
endif()

pulp_add_plugin(GuiZoo
    FORMATS           VST3 CLAP AU Standalone
    PLUGIN_NAME       "GuiZoo"
    BUNDLE_ID         "com.pulp.examples.gui-zoo"
    MANUFACTURER      "Pulp Examples"
    VERSION           "0.1.0"
    CATEGORY          Effect
    PLUGIN_CODE       "PGz1"
    MANUFACTURER_CODE "PlpX"
    DESIGN_WIDTH      720
    DESIGN_HEIGHT     760
    DESIGN_MIN_WIDTH  480
    DESIGN_MIN_HEIGHT 420
)
