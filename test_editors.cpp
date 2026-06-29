// Screenshot regression + bake for every plugin's dark Ink & Signal editor.
//
// Each case builds the param-bound editor, renders it (Skia), asserts the frame
// isn't blank, and compares it pixel-wise against the committed baseline in
// screenshots/<name>.png. Set PULP_BAKE_SCREENSHOTS=1 to (re)generate the
// baselines. Skips cleanly when Skia isn't in the SDK build or a baseline is
// missing. (gui-zoo has its own fixture test + baseline.)
#include <catch2/catch_test_macros.hpp>

#include "effect_editors.hpp"

#include <pulp/state/store.hpp>
#include <pulp/view/screenshot.hpp>
#include <pulp/view/screenshot_compare.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

using namespace pulp;
using namespace pulp::examples::classic;
namespace fs = std::filesystem;

#ifndef SCREENSHOTS_DIR
#define SCREENSHOTS_DIR "screenshots"
#endif

namespace {
bool bake_mode() {
    const char* e = std::getenv("PULP_BAKE_SCREENSHOTS");
    return e && *e && std::string(e) != "0";
}

template <class Proc, class Build>
void check_editor(Build build, const std::string& name) {
    Proc proc;
    state::StateStore store;
    proc.define_parameters(store);
    auto editor = build(store);
    const auto b = editor->bounds();
    const uint32_t w = static_cast<uint32_t>(b.width);
    const uint32_t h = static_cast<uint32_t>(b.height);
    const fs::path baseline = fs::path(SCREENSHOTS_DIR) / (name + ".png");

    auto png = view::render_to_png(*editor, w, h, 2.0f, view::ScreenshotBackend::skia);
    if (png.empty()) { SKIP("Skia raster backend unavailable in this build"); }
    // Relaxed "not blank" floor — some panels (e.g. the param-less MPE Spreader)
    // are sparse; the baseline comparison below is the real regression guard.
    const auto stats = view::analyze_screenshot_content(png);
    REQUIRE(stats.valid);
    REQUIRE(stats.unique_colors >= 4);

    if (bake_mode()) {
        REQUIRE(view::render_to_file(*editor, w, h, baseline.string(), 2.0f,
                                     view::ScreenshotBackend::skia));
        return;
    }
    if (!fs::exists(baseline)) {
        SKIP("no baseline " + baseline.string() +
             " — run with PULP_BAKE_SCREENSHOTS=1 to bake");
    }
    const auto tmp = fs::temp_directory_path() / (name + "-fresh.png");
    REQUIRE(view::render_to_file(*editor, w, h, tmp.string(), 2.0f,
                                 view::ScreenshotBackend::skia));
    const auto cmp = view::compare_screenshot_files(baseline.string(), tmp.string(), 24);
    INFO("baseline=" << baseline.string() << " similarity=" << cmp.similarity);
    REQUIRE(cmp.valid);
    REQUIRE(cmp.passes(0.97f));
    std::error_code ec; fs::remove(tmp, ec);
}
}  // namespace

TEST_CASE("MIDI Transpose editor matches baseline", "[editor]")      { check_editor<MidiTransposeProcessor>(build_midi_transpose_editor, "midi-transpose"); }
TEST_CASE("SysEx Echo editor matches baseline", "[editor]")          { check_editor<SysexEchoProcessor>(build_sysex_echo_editor, "sysex-echo"); }
TEST_CASE("MIDI Inspector editor matches baseline", "[editor]")      { check_editor<MidiInspectorProcessor>(build_midi_inspector_editor, "midi-inspector"); }
TEST_CASE("MPE Spreader editor matches baseline", "[editor]")        { check_editor<MpeSpreaderProcessor>(build_mpe_spreader_editor, "mpe-spreader"); }
TEST_CASE("State Memo editor matches baseline", "[editor]")          { check_editor<StateMemoProcessor>(build_state_memo_editor, "state-memo"); }
TEST_CASE("MonoSynth editor matches baseline", "[editor]")           { check_editor<MonoSynthProcessor>(build_mono_synth_editor, "mono-synth"); }
TEST_CASE("Synth With Presets editor matches baseline", "[editor]")  { check_editor<SynthWithPresetsProcessor>(build_synth_with_presets_editor, "synth-with-presets"); }
