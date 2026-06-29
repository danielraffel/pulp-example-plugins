#include <catch2/catch_test_macros.hpp>

#include "synth_with_presets.hpp"

#include <pulp/format/headless.hpp>
#include <pulp/format/validation_assertions.hpp>

#include <cmath>
#include <span>
#include <vector>

using namespace pulp;
using pulp::examples::classic::create_synth_with_presets;
using pulp::examples::classic::SynthWithPresetsProcessor;
using pulp::examples::classic::kSpProgram;
using pulp::examples::classic::kSpWaveform;
using pulp::examples::classic::kSpAttack;
using pulp::examples::classic::kSpRelease;
namespace v = pulp::format::validation;

namespace {
// Render `frames` samples carrying `in`, return left channel.
std::vector<float> render(format::HeadlessHost& host, midi::MidiBuffer in, int frames) {
    audio::Buffer<float> out(2, frames);
    const float* ip[2] = {nullptr, nullptr};
    audio::Buffer<float> silent(2, frames);
    ip[0] = silent.channel(0).data(); ip[1] = silent.channel(1).data();
    audio::BufferView<const float> iv(ip, 2, frames);
    auto ov = out.view();
    midi::MidiBuffer unused;
    host.process(ov, iv, in, unused, format::ProcessContext{});
    std::vector<float> r(frames);
    for (int i = 0; i < frames; ++i) r[i] = out.channel(0)[i];
    return r;
}
double rms(const std::vector<float>& x, int from, int to) {
    double a = 0; int c = 0;
    for (int i = from; i < to && i < (int)x.size(); ++i) { a += double(x[i]) * x[i]; ++c; }
    return std::sqrt(a / std::max(1, c));
}
int period(const std::vector<float>& x, int lo, int hi, int from) {
    double best = -1e30; int lag = lo;
    for (int L = lo; L <= hi; ++L) {
        double acc = 0; int c = 0;
        for (int n = from; n + L < (int)x.size(); ++n) { acc += double(x[n]) * x[n + L]; ++c; }
        double norm = c ? acc / c : 0;
        if (norm > best) { best = norm; lag = L; }
    }
    return lag;
}
}

TEST_CASE("SynthPresets is an instrument that sounds on note-on", "[synth-presets]") {
    format::HeadlessHost host(create_synth_with_presets);
    host.prepare(48000.0, 4096);
    auto d = host.descriptor();
    REQUIRE(d.category == format::PluginCategory::Instrument);
    REQUIRE(d.accepts_midi);

    midi::MidiBuffer in; in.add(midi::MidiEvent::note_on(0, 69, 100));  // A4 440 Hz
    auto out = render(host, in, 4096);
    REQUIRE(v::check_finite(out));
    REQUIRE(v::check_any_nonzero(out));
}

TEST_CASE("SynthPresets selecting a program loads its preset values", "[synth-presets]") {
    format::HeadlessHost host(create_synth_with_presets);
    host.prepare(48000.0, 256);
    host.state().set_value(kSpProgram, 1.0f);     // "Pad"
    render(host, midi::MidiBuffer{}, 256);         // a block applies the program
    const auto& pad = SynthWithPresetsProcessor::kFactory[1];
    REQUIRE(host.state().get_value(kSpWaveform) == pad.waveform);
    REQUIRE(host.state().get_value(kSpAttack) == pad.attack);
    REQUIRE(host.state().get_value(kSpRelease) == pad.release);
}

TEST_CASE("SynthPresets pitch bend raises the sounding pitch", "[synth-presets]") {
    auto sound = [](int bend14) {
        format::HeadlessHost host(create_synth_with_presets);
        host.prepare(48000.0, 8192);
        host.state().set_value(kSpProgram, 2.0f);   // sine, easiest to track
        midi::MidiBuffer in;
        in.add(midi::MidiEvent::note_on(0, 57, 110));        // A3 = 220 Hz
        if (bend14 >= 0) in.add(midi::MidiEvent::pitch_bend(0, (uint16_t)bend14));
        return render(host, in, 8192);
    };
    auto flat = sound(-1);                          // no bend: 220 Hz -> period ~218
    auto up = sound(16383);                          // +full bend (+2 st) -> ~247 Hz
    const int p_flat = period(flat, 150, 320, 3000);
    const int p_up = period(up, 150, 320, 3000);
    REQUIRE(p_up < p_flat);                          // higher pitch => shorter period
}

TEST_CASE("SynthPresets mod wheel adds vibrato (output moves)", "[synth-presets]") {
    auto energy_spread = [](int mod) {
        format::HeadlessHost host(create_synth_with_presets);
        host.prepare(48000.0, 8192);
        host.state().set_value(kSpProgram, 2.0f);
        midi::MidiBuffer in;
        in.add(midi::MidiEvent::note_on(0, 69, 110));
        if (mod > 0) in.add(midi::MidiEvent::cc(0, 1, (uint8_t)mod));   // mod wheel
        auto out = render(host, in, 8192);
        // With vibrato the steady tone is frequency-modulated, so a fixed-lag
        // autocorrelation against the no-vibrato period decorrelates more.
        return out;
    };
    auto dry = energy_spread(0);
    auto wet = energy_spread(127);
    double diff = 0;
    for (int n = 4000; n < 8000; ++n) diff += std::fabs(wet[n] - dry[n]);
    REQUIRE(diff > 1.0);                             // mod wheel changes the sound
}

TEST_CASE("SynthPresets program + params round-trip through host save/load", "[synth-presets]") {
    format::HeadlessHost host(create_synth_with_presets);
    host.prepare(48000.0, 64);
    host.state().set_value(kSpProgram, 1.0f);
    render(host, midi::MidiBuffer{}, 64);            // apply program 1
    host.state().set_value(kSpAttack, 0.5f);         // user edit on top of preset
    const auto saved = host.save_state();

    host.state().set_value(kSpProgram, 0.0f);
    host.state().set_value(kSpAttack, 0.001f);       // perturb
    REQUIRE(host.load_state(std::span<const uint8_t>(saved.data(), saved.size())));
    REQUIRE(host.state().get_value(kSpProgram) == 1.0f);
    REQUIRE(host.state().get_value(kSpAttack) == 0.5f);
}

TEST_CASE("SynthPresets rejects an out-of-range program in custom state", "[synth-presets]") {
    SynthWithPresetsProcessor p;
    std::vector<uint8_t> bad{0x07, 0, 0, 0};         // program 7 > 2
    REQUIRE_FALSE(p.deserialize_plugin_state(std::span<const uint8_t>(bad.data(), bad.size())));
    std::vector<uint8_t> ok{0x02, 0, 0, 0};          // program 2 valid
    REQUIRE(p.deserialize_plugin_state(std::span<const uint8_t>(ok.data(), ok.size())));
}
