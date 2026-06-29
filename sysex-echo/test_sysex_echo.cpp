#include <catch2/catch_test_macros.hpp>

#include "sysex_echo.hpp"

#include <pulp/format/headless.hpp>
#include <pulp/format/validation_assertions.hpp>

#include <cstdint>
#include <vector>

using namespace pulp;
using pulp::examples::classic::create_sysex_echo;
using pulp::examples::classic::kEchoEnabled;
namespace v = pulp::format::validation;

namespace {
midi::MidiBuffer run(format::HeadlessHost& host, midi::MidiBuffer in) {
    audio::Buffer<float> a(2, 64), b(2, 64);
    const float* ip[2] = {a.channel(0).data(), a.channel(1).data()};
    audio::BufferView<const float> iv(ip, 2, 64);
    auto ov = b.view();
    midi::MidiBuffer out;
    host.process(ov, iv, in, out, format::ProcessContext{});
    return out;
}
// A valid F0 .. F7 SysEx of the given total length (data bytes stay < 0x80).
std::vector<uint8_t> long_sysex(int total) {
    std::vector<uint8_t> s;
    s.reserve(total);
    s.push_back(0xF0);
    for (int i = 0; i < total - 2; ++i) s.push_back(static_cast<uint8_t>(i % 128));
    s.push_back(0xF7);
    return s;
}
}

TEST_CASE("SysExEcho declares a MIDI in/out effect", "[sysex-echo]") {
    format::HeadlessHost host(create_sysex_echo);
    auto d = host.descriptor();
    REQUIRE(d.category == format::PluginCategory::MidiEffect);
    REQUIRE(d.accepts_midi);
    REQUIRE(d.produces_midi);
}

TEST_CASE("SysExEcho round-trips a long SysEx payload unchanged", "[sysex-echo]") {
    format::HeadlessHost host(create_sysex_echo);
    host.prepare(48000.0, 64);
    host.state().set_value(kEchoEnabled, 1.0f);

    const auto payload = long_sysex(512);
    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));   // a short message rides along
    in.add_sysex(payload, 17);                       // non-zero sample offset
    auto out = run(host, in);

    // Short message preserved, and the SysEx payload + offset survive intact.
    REQUIRE(out.sysex().size() == 1);
    const auto& sx = out.sysex()[0];
    REQUIRE(sx.sample_offset == 17);
    std::vector<uint8_t> got(sx.data.begin(), sx.data.end());
    REQUIRE(got == payload);
    REQUIRE(v::check_midi_events_equal(in, out).ok);  // full buffer round-trips
}

TEST_CASE("SysExEcho with echo disabled drops SysEx but keeps short messages", "[sysex-echo]") {
    format::HeadlessHost host(create_sysex_echo);
    host.prepare(48000.0, 64);
    host.state().set_value(kEchoEnabled, 0.0f);

    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));
    in.add(midi::MidiEvent::cc(0, 7, 64));
    in.add_sysex(long_sysex(256), 0);
    auto out = run(host, in);

    REQUIRE(out.sysex().empty());        // SysEx dropped
    midi::MidiBuffer expected;           // the two short messages, unchanged
    expected.add(midi::MidiEvent::note_on(0, 60, 100));
    expected.add(midi::MidiEvent::cc(0, 7, 64));
    REQUIRE(v::check_midi_events_equal(expected, out).ok);
}

TEST_CASE("SysExEcho echoes every SysEx in a block, in order", "[sysex-echo]") {
    format::HeadlessHost host(create_sysex_echo);
    host.prepare(48000.0, 64);
    // Distinct payloads + offsets, including two at the same offset (pins order).
    midi::MidiBuffer in;
    in.add_sysex(long_sysex(8), 5);
    in.add_sysex(long_sysex(300), 5);
    in.add_sysex(long_sysex(64), 40);
    auto out = run(host, in);
    REQUIRE(out.sysex().size() == 3);    // all three, not just the first
    REQUIRE(v::check_midi_events_equal(in, out).ok);  // payloads + offsets, in order
}

TEST_CASE("SysExEcho echoes by default (no param touched)", "[sysex-echo]") {
    format::HeadlessHost host(create_sysex_echo);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add_sysex(long_sysex(32), 0);     // default kEchoEnabled is on
    auto out = run(host, in);
    REQUIRE(out.sysex().size() == 1);
}

TEST_CASE("SysExEcho round-trips an empty (F0 F7) payload", "[sysex-echo]") {
    format::HeadlessHost host(create_sysex_echo);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add_sysex({0xF0, 0xF7}, 0);
    auto out = run(host, in);
    REQUIRE(out.sysex().size() == 1);
    std::vector<uint8_t> got(out.sysex()[0].data.begin(), out.sysex()[0].data.end());
    REQUIRE(got == std::vector<uint8_t>{0xF0, 0xF7});
}
