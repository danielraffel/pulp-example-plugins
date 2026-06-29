#pragma once

// MIDI Inspector — a transparent MIDI pass-through that logs what flows through.
//
// Every message is forwarded unchanged (midi_in -> midi_out). Alongside, the
// plugin keeps per-type counts, a ring of the most recent events, and a dropped
// count for blocks that carry more loggable events than the ring can show.
// Per-type filters let the UI focus on notes / CC / other.
//
// Threading: the counts + ring are touched ONLY on the audio thread; each block
// publishes an immutable Snapshot via a TripleBuffer, and the UI/test thread
// reads the latest Snapshot through snapshot(). This is the SDK's prescribed
// latest-value-publication pattern, so there is no cross-thread data race.
// (The midi_out add()/add_sysex_copy pass-through is the host buffer's contract;
// faithful sysex echo copies bytes.)

#include <pulp/format/processor.hpp>
#include <pulp/runtime/triple_buffer.hpp>

#include <array>
#include <cstdint>
#include <memory>

namespace pulp::examples::classic {

enum MidiInspectorParams : state::ParamID {
    kLogNotes = 1,  // include note-on/off in the display log
    kLogCC    = 2,  // include control-change in the display log
    kLogOther = 3,  // include pitch-bend / sysex / everything else
};

class MidiInspectorProcessor : public format::Processor {
public:
    static constexpr int kRingCapacity = 64;

    enum class Kind : uint8_t { note_on, note_off, cc, pitch_bend, sysex, other };
    struct LogEntry { Kind kind; uint8_t channel; uint8_t a; uint8_t b; };
    struct Snapshot {
        uint32_t notes, ccs, pitch_bends, sysex, other, dropped, total;
        uint32_t ring_count;                 // valid entries in `recent`
        std::array<LogEntry, kRingCapacity> recent;
    };

    format::PluginDescriptor descriptor() const override {
        return {.name = "MidiInspector", .manufacturer = "Pulp Examples",
                .bundle_id = "com.pulp.examples.midi-inspector", .version = "0.1.0",
                .category = format::PluginCategory::MidiEffect,
                .input_buses = {}, .output_buses = {},
                .accepts_midi = true, .produces_midi = true};
    }

    void define_parameters(state::StateStore& store) override {
        store.add_parameter({.id = kLogNotes, .name = "Log Notes", .unit = "",
                             .range = {0.0f, 1.0f, 1.0f, 1.0f}});
        store.add_parameter({.id = kLogCC, .name = "Log CC", .unit = "",
                             .range = {0.0f, 1.0f, 1.0f, 1.0f}});
        store.add_parameter({.id = kLogOther, .name = "Log Other", .unit = "",
                             .range = {0.0f, 1.0f, 1.0f, 1.0f}});
    }

    void prepare(const format::PrepareContext&) override {
        reset_counts();
        published_.write(build_snapshot());   // publish a clean zero state
    }

    void process(audio::BufferView<float>& output,
                 const audio::BufferView<const float>& input,
                 midi::MidiBuffer& midi_in,
                 midi::MidiBuffer& midi_out,
                 const format::ProcessContext&) override {
        for (std::size_t ch = 0;
             ch < output.num_channels() && ch < input.num_channels(); ++ch) {
            auto in = input.channel(ch); auto out = output.channel(ch);
            const std::size_t n = std::min(in.size(), out.size());
            for (std::size_t i = 0; i < n; ++i) out[i] = in[i];
        }

        const bool log_notes = state().get_value(kLogNotes) >= 0.5f;
        const bool log_cc = state().get_value(kLogCC) >= 0.5f;
        const bool log_other = state().get_value(kLogOther) >= 0.5f;

        // The ring always retains the most recent kRingCapacity loggable events
        // (evicting the oldest). `dropped` counts per-block overflow: events a
        // UI polling once per block can't observe because they were overwritten
        // within the same block.
        uint32_t logged_this_block = 0;
        auto log = [&](Kind k, const midi::MidiEvent& ev) {
            ring_[ring_head_] = {k, ev.channel(), event_a(ev), event_b(ev)};
            ring_head_ = (ring_head_ + 1) % kRingCapacity;
            if (ring_count_ < kRingCapacity) ++ring_count_;
            ++logged_this_block;
        };

        for (const auto& ev : midi_in) {
            midi_out.add(ev);                                  // always pass through
            if (ev.is_note_on() && ev.velocity() > 0) {
                ++notes_;
                if (log_notes) log(Kind::note_on, ev);
            } else if (ev.is_note_off() || (ev.is_note_on() && ev.velocity() == 0)) {
                ++notes_;
                if (log_notes) log(Kind::note_off, ev);
            } else if (ev.is_cc()) {
                ++ccs_;
                if (log_cc) log(Kind::cc, ev);
            } else if (ev.is_pitch_bend()) {
                ++pitch_bends_;
                if (log_other) log(Kind::pitch_bend, ev);
            } else {
                ++other_;
                if (log_other) log(Kind::other, ev);
            }
        }
        if (logged_this_block > kRingCapacity)
            dropped_ += logged_this_block - kRingCapacity;

        for (const auto& sx : midi_in.sysex()) {
            midi_out.add_sysex_copy(sx.data.data(), sx.data.size(),
                                    sx.sample_offset, sx.timestamp);
            ++sysex_;
        }
        published_.write(build_snapshot());   // hand the latest state to the UI thread
    }

    /// Latest published state — safe to call from the UI/test thread.
    Snapshot snapshot() { return published_.read(); }

private:
    Snapshot build_snapshot() const {
        Snapshot s{};
        s.notes = notes_; s.ccs = ccs_; s.pitch_bends = pitch_bends_;
        s.sysex = sysex_; s.other = other_; s.dropped = dropped_;
        s.total = notes_ + ccs_ + pitch_bends_ + sysex_ + other_;
        s.ring_count = ring_count_;
        const uint32_t start =
            (ring_head_ + kRingCapacity - ring_count_) % kRingCapacity;
        for (uint32_t i = 0; i < ring_count_; ++i)
            s.recent[i] = ring_[(start + i) % kRingCapacity];
        return s;
    }
    static uint8_t event_a(const midi::MidiEvent& ev) {
        if (ev.is_cc()) return ev.cc_number();
        if (ev.is_note_on() || ev.is_note_off()) return ev.note();
        return ev.size() > 1 ? ev.data()[1] : 0;
    }
    static uint8_t event_b(const midi::MidiEvent& ev) {
        if (ev.is_cc()) return ev.cc_value();
        if (ev.is_note_on() || ev.is_note_off()) return ev.velocity();
        return ev.size() > 2 ? ev.data()[2] : 0;
    }
    void reset_counts() {
        notes_ = ccs_ = pitch_bends_ = sysex_ = other_ = dropped_ = 0;
        ring_head_ = ring_count_ = 0;
    }
    // All audio-thread-private; published to other threads only via published_.
    uint32_t notes_ = 0, ccs_ = 0, pitch_bends_ = 0, sysex_ = 0, other_ = 0, dropped_ = 0;
    std::array<LogEntry, kRingCapacity> ring_{};
    uint32_t ring_head_ = 0, ring_count_ = 0;
    runtime::TripleBuffer<Snapshot> published_;
};

inline std::unique_ptr<format::Processor> create_midi_inspector() {
    return std::make_unique<MidiInspectorProcessor>();
}

} // namespace pulp::examples::classic
