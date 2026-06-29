#include <catch2/catch_test_macros.hpp>

#include "gui_zoo.hpp"

#include <pulp/view/screenshot.hpp>
#include <pulp/view/screenshot_compare.hpp>

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

using namespace pulp::view;

namespace {

// The checked-in baseline lives next to this test. CMake passes its directory
// as GUI_ZOO_DIR so the test can find the baseline regardless of CWD.
std::string baseline_path() {
#ifdef GUI_ZOO_DIR
    return std::string(GUI_ZOO_DIR) + "/baseline.png";
#else
    return "baseline.png";
#endif
}

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

bool bake_mode() {
    const char* e = std::getenv("PULP_BAKE_SCREENSHOTS");
    return e && *e && std::string(e) != "0";
}

// Build the dark gallery and render at its self-computed size (the gallery is a
// multi-section board, not a fixed-size panel).
std::vector<uint8_t> render_gui_zoo() {
    auto view = pulp::examples::guizoo::build_gui_zoo(/*dark=*/true);
    const auto b = view->bounds();
    return render_to_png(*view, static_cast<uint32_t>(b.width),
                         static_cast<uint32_t>(b.height), 1.0f, ScreenshotBackend::skia);
}

} // namespace

TEST_CASE("gui-zoo renders deterministic, content-rich pixels", "[gui-zoo]") {
    auto png = render_gui_zoo();
    if (png.empty()) {
        SKIP("Skia raster screenshot backend unavailable on this platform");
    }
    // Content floor: the full widget gallery must be real, not blank.
    auto stats = analyze_screenshot_content(png);
    REQUIRE(stats.passes_content_floor());

    if (bake_mode()) {
        std::ofstream out(baseline_path(), std::ios::binary);
        out.write(reinterpret_cast<const char*>(png.data()),
                  static_cast<std::streamsize>(png.size()));
    }
}

TEST_CASE("gui-zoo matches its checked-in baseline", "[gui-zoo]") {
    if (bake_mode()) { SUCCEED("bake mode: baseline (re)generated above"); return; }
    auto png = render_gui_zoo();
    if (png.empty()) {
        SKIP("Skia raster screenshot backend unavailable on this platform");
    }
    auto baseline = read_file(baseline_path());
    if (baseline.empty()) {
        SKIP("no baseline.png checked in (run with PULP_BAKE_SCREENSHOTS=1 to create it)");
    }
    // Geometry-heavy layout should match closely; allow a tolerance for
    // platform font/AA differences (per-OS baselines if a lane diverges).
    auto cmp = compare_screenshots(baseline, png);
    REQUIRE(cmp.valid);
    REQUIRE(cmp.similarity > 0.97);
}
