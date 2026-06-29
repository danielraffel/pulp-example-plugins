// Headless render check for every example plugin's dark Ink & Signal editor.
// Builds the same param-bound tree the plugin shows and asserts it renders
// non-blank with real tonal range. Skips when the Skia raster backend isn't in
// the SDK build.
#include <catch2/catch_test_macros.hpp>

#include "effect_editors.hpp"

#include <pulp/state/store.hpp>
#include <pulp/view/screenshot.hpp>
#include <pulp/view/screenshot_compare.hpp>

#include <cstdint>
#include <memory>

using namespace pulp;
using namespace pulp::examples::classic;

namespace {
void check_renders(view::View& editor) {
    const auto b = editor.bounds();
    auto png = view::render_to_png(editor, static_cast<uint32_t>(b.width),
                                   static_cast<uint32_t>(b.height), 2.0f,
                                   view::ScreenshotBackend::skia);
    if (png.empty()) { SKIP("Skia raster backend unavailable in this build"); }
    const auto stats = view::analyze_screenshot_content(png);
    INFO("unique_colors=" << stats.unique_colors
         << " non_bg=" << stats.non_background_coverage);
    REQUIRE(stats.passes_content_floor(8, 2.0, 0.006, 0.95));
}

template <class Proc, class Build>
void run(Build build) {
    Proc proc;
    state::StateStore store;
    proc.define_parameters(store);
    auto editor = build(store);
    check_renders(*editor);
}
}  // namespace

TEST_CASE("MIDI Transpose editor renders", "[editor]")      { run<MidiTransposeProcessor>(build_midi_transpose_editor); }
TEST_CASE("SysEx Echo editor renders", "[editor]")          { run<SysexEchoProcessor>(build_sysex_echo_editor); }
TEST_CASE("MIDI Inspector editor renders", "[editor]")      { run<MidiInspectorProcessor>(build_midi_inspector_editor); }
TEST_CASE("State Memo editor renders", "[editor]")          { run<StateMemoProcessor>(build_state_memo_editor); }
TEST_CASE("MonoSynth editor renders", "[editor]")           { run<MonoSynthProcessor>(build_mono_synth_editor); }
TEST_CASE("Synth With Presets editor renders", "[editor]")  { run<SynthWithPresetsProcessor>(build_synth_with_presets_editor); }

// MPE Spreader has no parameters — its editor is a title-only status panel, so
// the content floor is relaxed to "not blank" rather than the widget floor.
TEST_CASE("MPE Spreader editor renders", "[editor]") {
    MpeSpreaderProcessor proc;
    state::StateStore store;
    proc.define_parameters(store);
    auto editor = build_mpe_spreader_editor(store);
    auto png = view::render_to_png(*editor, static_cast<uint32_t>(editor->bounds().width),
                                   static_cast<uint32_t>(editor->bounds().height), 2.0f,
                                   view::ScreenshotBackend::skia);
    if (png.empty()) { SKIP("Skia raster backend unavailable in this build"); }
    const auto stats = view::analyze_screenshot_content(png);
    REQUIRE(stats.valid);
    REQUIRE(stats.unique_colors >= 4);     // title + subtitle text on the themed panel
}
