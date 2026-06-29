#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "gain.hpp"

#include <pulp/format/headless.hpp>
#include <pulp/format/validation_assertions.hpp>

#include <cmath>
#include <utility>
#include <vector>

using namespace pulp;
using pulp::examples::classic::create_gain;
using pulp::examples::classic::kGainAmount;
using pulp::examples::classic::kGainPan;
namespace v = pulp::format::validation;

namespace {

// Render one block of a constant +1.0 stereo input; return {left, right}.
std::pair<std::vector<float>, std::vector<float>>
render(format::HeadlessHost& host, int frames) {
    audio::Buffer<float> in(2, frames), out(2, frames);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < frames; ++i) in.channel(ch)[i] = 1.0f;
    const float* ip[2] = {in.channel(0).data(), in.channel(1).data()};
    audio::BufferView<const float> iv(ip, 2, frames);
    auto ov = out.view();
    midi::MidiBuffer unused_in, unused_out;
    host.process(ov, iv, unused_in, unused_out, format::ProcessContext{});
    std::vector<float> l(frames), r(frames);
    for (int i = 0; i < frames; ++i) { l[i] = out.channel(0)[i]; r[i] = out.channel(1)[i]; }
    return {l, r};
}

float peak(const std::vector<float>& x) {
    float p = 0.0f;
    for (float s : x) p = std::max(p, std::fabs(s));
    return p;
}

} // namespace

TEST_CASE("Gain descriptor declares a stereo effect with two params", "[gain]") {
    format::HeadlessHost host(create_gain);
    auto d = host.descriptor();
    REQUIRE(d.category == format::PluginCategory::Effect);
    REQUIRE_FALSE(d.accepts_midi);
    REQUIRE(host.state().param_count() == 2);
}

TEST_CASE("Gain scales the signal linearly", "[gain]") {
    format::HeadlessHost host(create_gain);
    host.prepare(48000.0, 128);
    host.state().set_value(kGainPan, 0.0f);   // center: both channels share gl

    host.state().set_value(kGainAmount, 1.0f);
    auto [l1, r1] = render(host, 128);
    host.state().set_value(kGainAmount, 2.0f);
    auto [l2, r2] = render(host, 128);

    REQUIRE(v::check_finite(l2));
    REQUIRE(v::check_finite(r2));
    // Doubling the gain doubles the output at a fixed pan.
    REQUIRE(peak(l2) == Catch::Approx(2.0f * peak(l1)).margin(1e-4f));
    REQUIRE(peak(r2) == Catch::Approx(2.0f * peak(r1)).margin(1e-4f));
}

TEST_CASE("Gain center pan is equal power (−3 dB)", "[gain]") {
    format::HeadlessHost host(create_gain);
    host.prepare(48000.0, 64);
    host.state().set_value(kGainAmount, 1.0f);
    host.state().set_value(kGainPan, 0.0f);
    auto [l, r] = render(host, 64);
    // Center pan splits equally; each channel sits at cos(π/4) ≈ 0.7071.
    REQUIRE(peak(l) == Catch::Approx(0.70710678f).margin(1e-4f));
    REQUIRE(peak(r) == Catch::Approx(0.70710678f).margin(1e-4f));
}

TEST_CASE("Gain pan law moves the image and stays constant power", "[gain]") {
    format::HeadlessHost host(create_gain);
    host.prepare(48000.0, 64);
    host.state().set_value(kGainAmount, 1.0f);

    host.state().set_value(kGainPan, -1.0f);   // hard left
    auto [ll, lr] = render(host, 64);
    REQUIRE(peak(ll) == Catch::Approx(1.0f).margin(1e-4f));   // full level on the left
    REQUIRE(peak(lr) == Catch::Approx(0.0f).margin(1e-4f));   // silent on the right

    host.state().set_value(kGainPan, 1.0f);    // hard right
    auto [rl, rr] = render(host, 64);
    REQUIRE(peak(rl) == Catch::Approx(0.0f).margin(1e-4f));
    REQUIRE(peak(rr) == Catch::Approx(1.0f).margin(1e-4f));

    // Equal-power property: gl² + gr² == 1 at any pan position.
    host.state().set_value(kGainPan, -0.3f);
    auto [ml, mr] = render(host, 64);
    const float gl = peak(ml), gr = peak(mr);
    REQUIRE(gl * gl + gr * gr == Catch::Approx(1.0f).margin(1e-4f));
}

TEST_CASE("Gain output stays finite and bounded at extremes", "[gain]") {
    format::HeadlessHost host(create_gain);
    host.prepare(48000.0, 256);
    host.state().set_value(kGainAmount, 2.0f);   // max gain
    host.state().set_value(kGainPan, -1.0f);     // hard left

    auto [l, r] = render(host, 256);
    REQUIRE(v::check_finite(l));
    REQUIRE(v::check_finite(r));
    REQUIRE(v::check_peak_below(l, 2.0001f));     // gain 2 × full-left weight 1.0
    REQUIRE(v::check_state_round_trip(host).ok);
}
