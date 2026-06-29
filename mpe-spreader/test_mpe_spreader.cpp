#include <catch2/catch_test_macros.hpp>

#include "mpe_spreader.hpp"

#include <pulp/format/headless.hpp>

#include <vector>

using namespace pulp;
using pulp::examples::classic::create_mpe_spreader;

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
std::vector<midi::MidiEvent> collect(const midi::MidiBuffer& b) {
    return std::vector<midi::MidiEvent>(b.begin(), b.end());
}
}

TEST_CASE("MpeSpreader declares a MIDI in/out effect", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    auto d = host.descriptor();
    REQUIRE(d.category == format::PluginCategory::MidiEffect);
    REQUIRE(d.accepts_midi);
    REQUIRE(d.produces_midi);
}

TEST_CASE("MpeSpreader puts simultaneous notes on distinct member channels", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));
    in.add(midi::MidiEvent::note_on(0, 64, 100));
    auto e = collect(run(host, in));
    REQUIRE(e.size() == 2);
    REQUIRE(e[0].is_note_on()); REQUIRE(e[1].is_note_on());
    REQUIRE(e[0].channel() >= 1); REQUIRE(e[1].channel() >= 1);
    REQUIRE(e[0].channel() != e[1].channel());     // each note its own channel
}

TEST_CASE("MpeSpreader routes note-off to the note's channel and recycles it", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));   // -> channel A
    in.add(midi::MidiEvent::note_on(0, 64, 100));   // -> channel B
    in.add(midi::MidiEvent::note_off(0, 60, 0));    // frees A
    in.add(midi::MidiEvent::note_on(0, 67, 100));   // reuses A
    auto e = collect(run(host, in));
    REQUIRE(e.size() == 4);
    const int a = e[0].channel(), b = e[1].channel();
    REQUIRE(a != b);
    REQUIRE(e[2].is_note_off());
    REQUIRE(e[2].channel() == a);                   // note-off integrity
    REQUIRE(e[3].is_note_on());
    REQUIRE(e[3].channel() == a);                   // freed channel reused
}

TEST_CASE("MpeSpreader treats note-on velocity 0 as a note-off", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));
    in.add(midi::MidiEvent::note_on(0, 60, 0));     // = note-off
    auto e = collect(run(host, in));
    REQUIRE(e.size() == 2);
    REQUIRE(e[1].is_note_off());
    REQUIRE(e[1].channel() == e[0].channel());      // released on the same channel
}

TEST_CASE("MpeSpreader passes non-note messages through unchanged", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add(midi::MidiEvent::cc(3, 7, 64));
    auto e = collect(run(host, in));
    REQUIRE(e.size() == 1);
    REQUIRE(e[0].channel() == 3);                   // original channel preserved
    REQUIRE_FALSE(e[0].is_note_on());
}

TEST_CASE("MpeSpreader sustains a note across process blocks", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer on; on.add(midi::MidiEvent::note_on(0, 60, 100));
    auto e_on = collect(run(host, on));               // block 1: note held
    REQUIRE(e_on.size() == 1);
    const int ch = e_on[0].channel();
    REQUIRE(ch >= 1);

    midi::MidiBuffer off; off.add(midi::MidiEvent::note_off(0, 60, 0));
    auto e_off = collect(run(host, off));             // block 2: released
    REQUIRE(e_off.size() == 1);
    REQUIRE(e_off[0].is_note_off());
    REQUIRE(e_off[0].channel() == ch);                // state persisted across blocks
}

TEST_CASE("MpeSpreader retrigger reuses one channel and frees it once", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_on(0, 60, 100));     // alloc ch A
    in.add(midi::MidiEvent::note_on(0, 60, 110));     // retrigger: SAME channel, no leak
    in.add(midi::MidiEvent::note_off(0, 60, 0));      // frees A
    in.add(midi::MidiEvent::note_on(0, 64, 100));     // new note reuses A
    auto e = collect(run(host, in));
    REQUIRE(e.size() == 4);
    REQUIRE(e[0].channel() == e[1].channel());        // retrigger reuses, not leaks
    REQUIRE(e[3].channel() == e[0].channel());        // freed once, reusable
}

TEST_CASE("MpeSpreader lone note-off passes through without corrupting state", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    in.add(midi::MidiEvent::note_off(0, 60, 0));      // never turned on
    in.add(midi::MidiEvent::note_on(0, 62, 100));     // should still get member ch 1
    auto e = collect(run(host, in));
    REQUIRE(e.size() == 2);
    REQUIRE(e[0].is_note_off());
    REQUIRE(e[0].channel() == 0);                     // passed through unchanged
    REQUIRE(e[1].channel() == 1);                     // state uncorrupted
}

TEST_CASE("MpeSpreader recovers a member channel after exhaustion", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    for (int n = 0; n < 16; ++n)                       // 15 members + 1 overflow on ch0
        in.add(midi::MidiEvent::note_on(0, static_cast<uint8_t>(60 + n), 100));
    in.add(midi::MidiEvent::note_off(0, 60, 0));       // free the first member
    in.add(midi::MidiEvent::note_on(0, 90, 100));      // new note takes the freed member
    auto e = collect(run(host, in));
    const int freed = e[0].channel();                  // member used by note 60
    REQUIRE(freed >= 1);
    REQUIRE(e.back().is_note_on());
    REQUIRE(e.back().channel() == freed);              // reclaimed, not ch0/overflow
}

TEST_CASE("MpeSpreader preserves sample offsets", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    auto on = midi::MidiEvent::note_on(0, 60, 100); on.sample_offset = 23;
    in.add(on);
    auto e = collect(run(host, in));
    REQUIRE(e.size() == 1);
    REQUIRE(e[0].sample_offset == 23);
}

TEST_CASE("MpeSpreader stays valid when the zone is exhausted", "[mpe-spreader]") {
    format::HeadlessHost host(create_mpe_spreader);
    host.prepare(48000.0, 64);
    midi::MidiBuffer in;
    for (int n = 0; n < 18; ++n)                    // more notes than 15 members
        in.add(midi::MidiEvent::note_on(0, static_cast<uint8_t>(60 + n), 100));
    auto e = collect(run(host, in));
    REQUIRE(e.size() == 18);
    int distinct_members = 0;
    bool seen[16] = {};
    for (const auto& ev : e) {
        REQUIRE(ev.channel() <= 15);                // always a legal channel
        if (ev.channel() >= 1 && !seen[ev.channel()]) { seen[ev.channel()] = true; ++distinct_members; }
    }
    REQUIRE(distinct_members == 15);                // all 15 members used before fallback
}
