#include <catch2/catch_test_macros.hpp>

#include "midi_inspector.hpp"

#include <pulp/format/headless.hpp>

using namespace pulp;
using pulp::examples::classic::create_midi_inspector;
using pulp::examples::classic::MidiInspectorProcessor;
using pulp::examples::classic::kLogNotes;
using pulp::examples::classic::kLogCC;
using Kind = MidiInspectorProcessor::Kind;

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
}

TEST_CASE("MidiInspector passes every message through unchanged", "[midi-inspector]") {
    format::HeadlessHost host(create_midi_inspector);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));
    auto cc = midi::MidiEvent::cc(0, 7, 64);
    cc.sample_offset = 11;                       // a non-zero offset must survive
    in.add(cc);
    in.add_sysex({0xF0, 0x7D, 0x01, 0xF7}, 0);
    auto out = run(host, in);
    REQUIRE(out.size() == 2);                    // both short messages pass
    REQUIRE(out.sysex().size() == 1);            // sysex passes
    REQUIRE(out[0].is_note_on());
    REQUIRE(out[1].is_cc());
    REQUIRE(out[1].sample_offset == 11);         // offset preserved
}

TEST_CASE("MidiInspector counts events by type", "[midi-inspector]") {
    format::HeadlessHost host(create_midi_inspector);
    host.prepare(48000.0, 64);
    auto* insp = host.processor_as<MidiInspectorProcessor>();
    REQUIRE(insp != nullptr);

    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));
    in.add(midi::MidiEvent::note_off(0, 60, 0));
    in.add(midi::MidiEvent::cc(0, 1, 40));
    in.add(midi::MidiEvent::cc(0, 7, 90));
    in.add(midi::MidiEvent::pitch_bend(0, 9000));
    in.add_sysex({0xF0, 0x10, 0xF7}, 0);
    run(host, in);

    const auto s = insp->snapshot();
    REQUIRE(s.notes == 2);
    REQUIRE(s.ccs == 2);
    REQUIRE(s.pitch_bends == 1);
    REQUIRE(s.sysex == 1);
    REQUIRE(s.total == 6);                       // 2 notes + 2 cc + 1 bend + 1 sysex
    REQUIRE(s.dropped == 0);
}

TEST_CASE("MidiInspector filters which events reach the display log", "[midi-inspector]") {
    format::HeadlessHost host(create_midi_inspector);
    host.prepare(48000.0, 64);
    host.state().set_value(kLogNotes, 0.0f);     // hide notes from the log
    auto* insp = host.processor_as<MidiInspectorProcessor>();
    REQUIRE(insp != nullptr);

    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));
    in.add(midi::MidiEvent::note_on(0, 64, 100));
    in.add(midi::MidiEvent::cc(0, 7, 64));
    run(host, in);

    const auto s = insp->snapshot();
    REQUIRE(s.notes == 2);                       // still counted (totals are unfiltered)
    REQUIRE(s.ccs == 1);
    REQUIRE(s.ring_count == 1);                  // only the CC reached the display ring
    REQUIRE(s.recent[0].kind == Kind::cc);
}

TEST_CASE("MidiInspector display ring is ordered oldest -> newest and wraps", "[midi-inspector]") {
    format::HeadlessHost host(create_midi_inspector);
    host.prepare(48000.0, 4096);
    auto* insp = host.processor_as<MidiInspectorProcessor>();
    REQUIRE(insp != nullptr);

    const int cap = MidiInspectorProcessor::kRingCapacity;
    midi::MidiBuffer in;                                  // cap + 10 distinct CC values
    for (int i = 0; i < cap + 10; ++i)
        in.add(midi::MidiEvent::cc(0, 1, static_cast<uint8_t>(i)));
    run(host, in);

    const auto s = insp->snapshot();
    REQUIRE(s.ring_count == static_cast<uint32_t>(cap));
    REQUIRE(s.recent[0].b == 10);                         // oldest retained = event #10
    REQUIRE(s.recent[cap - 1].b == cap + 9);              // newest = last event
}

TEST_CASE("MidiInspector ring persists and wraps across process blocks", "[midi-inspector]") {
    format::HeadlessHost host(create_midi_inspector);
    host.prepare(48000.0, 64);
    auto* insp = host.processor_as<MidiInspectorProcessor>();
    REQUIRE(insp != nullptr);

    midi::MidiBuffer b1;
    for (int v : {1, 2, 3, 4, 5}) b1.add(midi::MidiEvent::cc(0, 1, static_cast<uint8_t>(v)));
    run(host, b1);
    midi::MidiBuffer b2;
    for (int v : {100, 101, 102}) b2.add(midi::MidiEvent::cc(0, 1, static_cast<uint8_t>(v)));
    run(host, b2);

    const auto s = insp->snapshot();
    REQUIRE(s.ccs == 8);                                  // counts persist across blocks
    REQUIRE(s.ring_count == 8);
    REQUIRE(s.recent[0].b == 1);                          // first block's oldest
    REQUIRE(s.recent[7].b == 102);                        // second block's newest
}

TEST_CASE("MidiInspector CC filter hides CCs from the ring but still counts them", "[midi-inspector]") {
    format::HeadlessHost host(create_midi_inspector);
    host.prepare(48000.0, 64);
    host.state().set_value(kLogCC, 0.0f);                 // hide CC from the log
    auto* insp = host.processor_as<MidiInspectorProcessor>();
    REQUIRE(insp != nullptr);

    midi::MidiBuffer in;
    in.add(midi::MidiEvent::cc(0, 7, 64));
    in.add(midi::MidiEvent::cc(0, 1, 40));
    in.add(midi::MidiEvent::note_on(0, 60, 100));
    run(host, in);

    const auto s = insp->snapshot();
    REQUIRE(s.ccs == 2);                                  // counted
    REQUIRE(s.ring_count == 1);                           // only the note reached the ring
    REQUIRE(s.recent[0].kind == Kind::note_on);
}

TEST_CASE("MidiInspector reports a dropped count when a block overflows the ring", "[midi-inspector]") {
    format::HeadlessHost host(create_midi_inspector);
    host.prepare(48000.0, 4096);
    auto* insp = host.processor_as<MidiInspectorProcessor>();
    REQUIRE(insp != nullptr);

    const int cap = MidiInspectorProcessor::kRingCapacity;
    midi::MidiBuffer in;
    for (int i = 0; i < cap + 20; ++i)
        in.add(midi::MidiEvent::cc(0, 1, static_cast<uint8_t>(i % 128)));
    run(host, in);

    const auto s = insp->snapshot();
    REQUIRE(s.ccs == static_cast<uint32_t>(cap + 20));   // all counted
    REQUIRE(s.ring_count == static_cast<uint32_t>(cap)); // ring full
    REQUIRE(s.dropped == 20);                            // display overflow
}
