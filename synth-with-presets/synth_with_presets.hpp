#pragma once

// Synth With Presets — a monophonic instrument with a factory preset bank.
//
// Builds on the minimal osc + ADSR voice (see mono-synth) and adds the pieces
// the preset/instrument contract needs: a Program selector that loads a factory
// preset into the timbre parameters (clamped to their ranges), pitch-bend (+/- 2
// semitones), and mod-wheel (CC1) vibrato. Program + parameters round-trip
// through the host's save/load path. Built only on Pulp's own pulp::signal
// primitives (Oscillator + Adsr); clean-room.

#include <pulp/format/processor.hpp>
#include <pulp/view/view.hpp>
#include <pulp/signal/adsr.hpp>
#include <pulp/signal/oscillator.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace pulp::examples::classic {

enum SynthPresetParams : state::ParamID {
    kSpProgram  = 1,  // 0..2 factory preset selector
    kSpWaveform = 2,  // 0 sine, 1 saw, 2 square, 3 triangle
    kSpAttack   = 3,  // seconds
    kSpRelease  = 4,  // seconds
};

// Defined out-of-line in synth_with_presets_editor.hpp (included at the bottom of this file).
// Forward-declared so the editor the screenshot tests render is the same
// tree the host receives from create_view().
std::unique_ptr<view::View> build_synth_with_presets_editor(state::StateStore& store);

class SynthWithPresetsProcessor : public format::Processor {
public:
    // Hand the host our dark Ink & Signal editor; the framework owns the
    // returned tree and may call this once per attached editor window.
    std::unique_ptr<view::View> create_view() override { return build_synth_with_presets_editor(state()); }

    struct Preset { float waveform, attack, release; };
    static constexpr int kNumPrograms = 3;
    static constexpr std::array<Preset, kNumPrograms> kFactory{{
        {1.0f, 0.005f, 0.20f},   // 0: "Pluck"  — saw, fast attack, short tail
        {2.0f, 0.30f,  0.80f},   // 1: "Pad"    — square, slow attack, long tail
        {0.0f, 0.02f,  0.40f},   // 2: "Sine"   — sine, med
    }};

    format::PluginDescriptor descriptor() const override {
        return {.name = "SynthPresets", .manufacturer = "Pulp Examples",
                .bundle_id = "com.pulp.examples.synth-with-presets", .version = "0.1.0",
                .category = format::PluginCategory::Instrument,
                .input_buses = {}, .output_buses = {{"Audio Out", 2}},
                .accepts_midi = true, .produces_midi = false};
    }

    void define_parameters(state::StateStore& store) override {
        store.add_parameter({.id = kSpProgram, .name = "Program", .unit = "",
                             .range = {0.0f, float(kNumPrograms - 1), 0.0f, 1.0f}});
        store.add_parameter({.id = kSpWaveform, .name = "Waveform", .unit = "",
                             .range = {0.0f, 3.0f, 1.0f, 1.0f}});
        store.add_parameter({.id = kSpAttack, .name = "Attack", .unit = "s",
                             .range = {0.001f, 2.0f, 0.005f, 0.0f}});
        store.add_parameter({.id = kSpRelease, .name = "Release", .unit = "s",
                             .range = {0.001f, 3.0f, 0.20f, 0.0f}});
    }

    void prepare(const format::PrepareContext& ctx) override {
        sample_rate_ = static_cast<float>(ctx.sample_rate);
        osc_.set_sample_rate(sample_rate_);
        lfo_.set_sample_rate(sample_rate_);
        lfo_.set_waveform(signal::Oscillator::Waveform::sine);
        lfo_.set_frequency(5.0f);
        env_.set_sample_rate(sample_rate_);
        osc_.reset(); lfo_.reset(); env_.reset();
        active_note_ = -1; bend_ = 0.0f; mod_ = 0.0f; last_program_ = -1;
    }

    void release() override { osc_.reset(); env_.reset(); active_note_ = -1; }

    // Load a factory preset into the timbre parameters (clamped via set_value).
    void apply_program(int program) {
        program = std::clamp(program, 0, kNumPrograms - 1);
        const Preset& p = kFactory[static_cast<std::size_t>(program)];
        state().set_value(kSpWaveform, p.waveform);
        state().set_value(kSpAttack, p.attack);
        state().set_value(kSpRelease, p.release);
        last_program_ = program;
    }

    void process(audio::BufferView<float>& output,
                 const audio::BufferView<const float>&,
                 midi::MidiBuffer& midi_in,
                 midi::MidiBuffer&,
                 const format::ProcessContext&) override {
        // A program change loads its preset into the timbre params (once).
        const int program = static_cast<int>(state().get_value(kSpProgram) + 0.5f);
        if (program != last_program_) apply_program(program);

        osc_.set_waveform(waveform_from_param(state().get_value(kSpWaveform)));
        env_.set_params({state().get_value(kSpAttack), 0.05f, 0.8f,
                         state().get_value(kSpRelease)});

        const std::size_t frames = output.num_samples();
        midi_in.sort();
        std::size_t event = 0;
        const std::size_t n = midi_in.size();
        for (std::size_t i = 0; i < frames; ++i) {
            while (event < n &&
                   static_cast<std::size_t>(std::max(0, midi_in[event].sample_offset)) <= i) {
                apply_event(midi_in[event]);
                ++event;
            }
            // Pitch = note +/- 2 st bend + mod-wheel vibrato (up to +/- 0.5 st).
            const float vib = lfo_.next() * mod_ * 0.5f;
            const float semis = bend_ * 2.0f + vib;
            osc_.set_frequency(base_hz_ * std::pow(2.0f, semis / 12.0f));
            const float s = osc_.next() * env_.next() * velocity_gain_;
            for (std::size_t ch = 0; ch < output.num_channels(); ++ch)
                output.channel(ch)[i] = s;
        }
        for (; event < n; ++event) apply_event(midi_in[event]);
    }

    // The program and all timbre parameters round-trip through the host's
    // parameter store. This custom blob only records which program was active
    // so that, on restore, we DON'T re-apply the factory preset over the user's
    // edited values: we sync last_program_ to the restored index instead of
    // calling apply_program(). (Re-applying here would clobber edits and is the
    // classic "preset reloads on recall" bug.) Fail-safe on bad input.
    std::vector<uint8_t> serialize_plugin_state() const override {
        std::vector<uint8_t> v;
        const uint32_t prog = static_cast<uint32_t>(std::max(0, last_program_));
        for (int i = 0; i < 4; ++i) v.push_back(uint8_t((prog >> (8 * i)) & 0xFF));
        return v;
    }
    bool deserialize_plugin_state(std::span<const uint8_t> d) override {
        if (d.empty()) return true;                 // nothing saved yet
        if (d.size() < 4) return false;
        uint32_t prog = uint32_t(d[0]) | (uint32_t(d[1]) << 8) |
                        (uint32_t(d[2]) << 16) | (uint32_t(d[3]) << 24);
        if (prog >= static_cast<uint32_t>(kNumPrograms)) return false;  // range check
        last_program_ = static_cast<int>(prog);     // sync only — do NOT re-apply
        return true;
    }

private:
    void apply_event(const midi::MidiEvent& ev) {
        if (ev.is_note_on() && ev.velocity() > 0) {
            active_note_ = ev.note();
            active_channel_ = ev.channel();
            base_hz_ = note_to_hz(ev.note());
            velocity_gain_ = ev.velocity() / 127.0f;
            env_.note_on();
        } else if (ev.is_note_off() || (ev.is_note_on() && ev.velocity() == 0)) {
            if (ev.note() == active_note_ && ev.channel() == active_channel_) {
                env_.note_off();
                active_note_ = -1;
            }
        } else if (ev.is_pitch_bend()) {
            const uint8_t* b = ev.data();             // [status][LSB][MSB], 14-bit
            const int value = int(b[1]) | (int(b[2]) << 7);
            bend_ = (value - 8192) / 8192.0f;         // -1..1
        } else if (ev.is_cc() && ev.cc_number() == 1) {
            mod_ = ev.cc_value() / 127.0f;            // mod wheel -> vibrato depth
        }
    }
    static float note_to_hz(int note) {
        return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }
    static signal::Oscillator::Waveform waveform_from_param(float v) {
        switch (static_cast<int>(v + 0.5f)) {
            case 0:  return signal::Oscillator::Waveform::sine;
            case 2:  return signal::Oscillator::Waveform::square;
            case 3:  return signal::Oscillator::Waveform::triangle;
            default: return signal::Oscillator::Waveform::saw;
        }
    }
    float sample_rate_ = 48000.0f;
    signal::Oscillator osc_, lfo_;
    signal::Adsr env_;
    int active_note_ = -1, active_channel_ = -1, last_program_ = -1;
    float base_hz_ = 440.0f, velocity_gain_ = 0.0f, bend_ = 0.0f, mod_ = 0.0f;
};

inline std::unique_ptr<format::Processor> create_synth_with_presets() {
    return std::make_unique<SynthWithPresetsProcessor>();
}

} // namespace pulp::examples::classic

// Pulls in the inline definition of build_synth_with_presets_editor (declared above) so create_view()
// links in the plugin adapter and the headless tests alike. After the class so
// the editor header sees a complete definition; its re-include is a no-op.
#include "synth_with_presets_editor.hpp"
