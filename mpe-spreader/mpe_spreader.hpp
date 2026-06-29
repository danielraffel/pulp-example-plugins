#pragma once

// MPE Spreader — give every held note its own MPE member channel.
//
// A MIDI effect that turns ordinary (single-channel) note input into MPE: each
// note-on is assigned a free member channel (MPE Lower Zone members are
// channels 2..16, i.e. 0-indexed 1..15; channel 1 / index 0 is the master), and
// the matching note-off is routed back out on that same channel. This is the
// channel-assignment + note-off-integrity core that per-note pitch/pressure
// expression builds on. Non-note messages pass through unchanged.
//
// Scope (kept deliberately small): the note->channel map is keyed by note
// number only, so this assumes single-channel note input — the same note on two
// different input channels would share one mapping. On a transport reset the
// held-note bookkeeping is cleared (the host is expected to pair reset with
// all-notes-off); a production spreader would flush note-offs for held notes.

#include <pulp/format/processor.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>

namespace pulp::examples::classic {

class MpeSpreaderProcessor : public format::Processor {
public:
    static constexpr int kFirstMember = 1;   // 0-indexed channel 1 == MIDI ch 2
    static constexpr int kLastMember = 15;   // 0-indexed channel 15 == MIDI ch 16

    format::PluginDescriptor descriptor() const override {
        return {
            .name = "MpeSpreader",
            .manufacturer = "Pulp Examples",
            .bundle_id = "com.pulp.examples.mpe-spreader",
            .version = "0.1.0",
            .category = format::PluginCategory::MidiEffect,
            .input_buses = {},
            .output_buses = {},
            .accepts_midi = true,
            .produces_midi = true,
        };
    }

    void define_parameters(state::StateStore&) override {}

    void prepare(const format::PrepareContext&) override { reset_state(); }

    void process(audio::BufferView<float>& output,
                 const audio::BufferView<const float>& input,
                 midi::MidiBuffer& midi_in,
                 midi::MidiBuffer& midi_out,
                 const format::ProcessContext& ctx) override {
        for (std::size_t ch = 0;
             ch < output.num_channels() && ch < input.num_channels(); ++ch) {
            auto in = input.channel(ch);
            auto out = output.channel(ch);
            const std::size_t n = std::min(in.size(), out.size());
            for (std::size_t i = 0; i < n; ++i) out[i] = in[i];
            for (std::size_t i = n; i < out.size(); ++i) out[i] = 0.0f;
        }
        if (ctx.should_reset_dsp_state()) reset_state();

        for (const auto& ev : midi_in) {
            const bool real_on = ev.is_note_on() && ev.velocity() > 0;
            const bool real_off =
                ev.is_note_off() || (ev.is_note_on() && ev.velocity() == 0);

            if (real_on) {
                const uint8_t note = ev.note();
                int ch = note_channel_[note];
                if (ch < 0) {                       // assign a fresh channel
                    ch = allocate_member();
                    note_channel_[note] = static_cast<int8_t>(ch);
                    if (ch >= kFirstMember) member_used_[ch] = true;
                }
                emit(midi_out, midi::MidiEvent::note_on(
                                   static_cast<uint8_t>(ch), note, ev.velocity()), ev);
            } else if (real_off) {
                const uint8_t note = ev.note();
                const int ch = note_channel_[note];
                if (ch < 0) { midi_out.add(ev); continue; }  // unknown note: pass thru
                emit(midi_out, midi::MidiEvent::note_off(
                                   static_cast<uint8_t>(ch), note, ev.velocity()), ev);
                if (ch >= kFirstMember) member_used_[ch] = false;
                note_channel_[note] = -1;
            } else {
                midi_out.add(ev);                    // CC / bend / etc. pass through
            }
        }
        for (const auto& sx : midi_in.sysex())
            midi_out.add_sysex_copy(sx.data.data(), sx.data.size(),
                                    sx.sample_offset, sx.timestamp);
    }

private:
    static void emit(midi::MidiBuffer& out, midi::MidiEvent e, const midi::MidiEvent& src) {
        e.sample_offset = src.sample_offset;
        out.add(e);
    }
    // Lowest free member channel; falls back to the master channel (0) when the
    // zone is exhausted so the note still sounds (just without isolation).
    int allocate_member() {
        for (int c = kFirstMember; c <= kLastMember; ++c)
            if (!member_used_[c]) return c;
        return 0;
    }
    void reset_state() {
        note_channel_.fill(-1);
        member_used_.fill(false);
    }
    std::array<int8_t, 128> note_channel_{};   // note -> assigned channel, -1 = free
    std::array<bool, 16> member_used_{};        // channel index in use?
};

inline std::unique_ptr<format::Processor> create_mpe_spreader() {
    return std::make_unique<MpeSpreaderProcessor>();
}

} // namespace pulp::examples::classic
