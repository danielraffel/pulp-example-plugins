#pragma once

// MIDI Transpose — shift incoming note pitches by a semitone offset.
//
// A minimal MIDI effect: it reads note-on/note-off messages from the
// input, adds a transpose offset (in semitones, clamped to the 0..127 note
// range) and emits them on the output; every other message (CC, pitch bend,
// program change, SysEx) passes through unchanged. There is no creative DSP
// here — it is a transparent demonstration of Pulp's MIDI-effect contract
// (accepts_midi / produces_midi, the midi_in -> midi_out path in process()).

#include <pulp/format/processor.hpp>
#include <pulp/view/view.hpp>

#include <algorithm>
#include <memory>

namespace pulp::examples::classic {

enum MidiTransposeParams : state::ParamID {
    kSemitones = 1,  // -24 .. +24
    kOctave    = 2,  // -3 .. +3 (added to the semitone shift, ×12)
};

// Defined out-of-line in midi_transpose_editor.hpp (included at the bottom of this file).
// Forward-declared so the editor the screenshot tests render is the same
// tree the host receives from create_view().
std::unique_ptr<view::View> build_midi_transpose_editor(state::StateStore& store);

class MidiTransposeProcessor : public format::Processor {
public:
    // Hand the host our dark Ink & Signal editor; the framework owns the
    // returned tree and may call this once per attached editor window.
    std::unique_ptr<view::View> create_view() override { return build_midi_transpose_editor(state()); }

    format::PluginDescriptor descriptor() const override {
        return {
            .name = "MidiTranspose",
            .manufacturer = "Pulp Examples",
            .bundle_id = "com.pulp.examples.midi-transpose",
            .version = "0.1.0",
            .category = format::PluginCategory::MidiEffect,
            .input_buses = {},
            .output_buses = {},
            .accepts_midi = true,
            .produces_midi = true,
        };
    }

    void define_parameters(state::StateStore& store) override {
        store.add_parameter({
            .id = kSemitones,
            .name = "Semitones",
            .unit = "st",
            .range = {-24.0f, 24.0f, 0.0f, 1.0f},  // stepped, integer semitones
        });
        store.add_parameter({
            .id = kOctave,
            .name = "Octave",
            .unit = "oct",
            .range = {-3.0f, 3.0f, 0.0f, 1.0f},  // stepped, integer octaves
        });
    }

    void prepare(const format::PrepareContext&) override {}

    void process(audio::BufferView<float>& output,
                 const audio::BufferView<const float>& input,
                 midi::MidiBuffer& midi_in,
                 midi::MidiBuffer& midi_out,
                 const format::ProcessContext&) override {
        // Pure MIDI effect: pass audio through untouched (if any bus exists).
        for (std::size_t ch = 0;
             ch < output.num_channels() && ch < input.num_channels(); ++ch) {
            auto in = input.channel(ch);
            auto out = output.channel(ch);
            for (std::size_t i = 0; i < out.size(); ++i) out[i] = in[i];
        }

        // Total shift combines the semitone offset with whole-octave steps.
        const int offset = static_cast<int>(state().get_value(kSemitones))
                         + 12 * static_cast<int>(state().get_value(kOctave));

        for (const auto& ev : midi_in) {
            if ((ev.is_note_on() || ev.is_note_off()) && offset != 0) {
                const int shifted = static_cast<int>(ev.note()) + offset;
                if (shifted < 0 || shifted > 127) {
                    // Transposed out of range — drop the note rather than wrap.
                    continue;
                }
                auto shifted_ev =
                    ev.is_note_on()
                        ? midi::MidiEvent::note_on(ev.channel(),
                                                   static_cast<uint8_t>(shifted),
                                                   ev.velocity())
                        : midi::MidiEvent::note_off(ev.channel(),
                                                    static_cast<uint8_t>(shifted),
                                                    ev.velocity());
                shifted_ev.sample_offset = ev.sample_offset;
                midi_out.add(shifted_ev);
            } else {
                midi_out.add(ev);
            }
        }

        // SysEx and other sidecar payloads pass through verbatim.
        for (const auto& sx : midi_in.sysex()) {
            midi_out.add_sysex_copy(sx.data.data(), sx.data.size(),
                                    sx.sample_offset, sx.timestamp);
        }
    }

private:
};

inline std::unique_ptr<format::Processor> create_midi_transpose() {
    return std::make_unique<MidiTransposeProcessor>();
}

} // namespace pulp::examples::classic

// Pulls in the inline definition of build_midi_transpose_editor (declared above) so create_view()
// links in the plugin adapter and the headless tests alike. After the class so
// the editor header sees a complete definition; its re-include is a no-op.
#include "midi_transpose_editor.hpp"
