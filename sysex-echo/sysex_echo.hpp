#pragma once

// SysEx Echo — a MIDI effect that round-trips System Exclusive messages.
//
// Short MIDI messages always pass through unchanged; when echo is enabled, every
// incoming SysEx payload is copied to the output (preserving its sample offset
// and timestamp). This is a transparent demonstration of Pulp's SysEx contract —
// variable-length F0..F7 payloads surviving the midi_in -> midi_out path.
//
// For clarity this example heap-copies each payload via add_sysex(). A real
// MIDI effect that must stay allocation-free on the audio thread should instead
// reserve a payload pool and use add_sysex_copy() under set_realtime_capacity_limit.

#include <pulp/format/processor.hpp>
#include <pulp/view/view.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace pulp::examples::classic {

enum SysexEchoParams : state::ParamID {
    kEchoEnabled = 1,  // 0 = drop SysEx, 1 = echo it to the output
};

// Defined out-of-line in sysex_echo_editor.hpp (included at the bottom of this file).
// Forward-declared so the editor the screenshot tests render is the same
// tree the host receives from create_view().
std::unique_ptr<view::View> build_sysex_echo_editor(state::StateStore& store);

class SysexEchoProcessor : public format::Processor {
public:
    // Hand the host our dark Ink & Signal editor; the framework owns the
    // returned tree and may call this once per attached editor window.
    std::unique_ptr<view::View> create_view() override { return build_sysex_echo_editor(state()); }

    format::PluginDescriptor descriptor() const override {
        return {
            .name = "SysExEcho",
            .manufacturer = "Pulp Examples",
            .bundle_id = "com.pulp.examples.sysex-echo",
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
            .id = kEchoEnabled,
            .name = "Echo SysEx",
            .unit = "",
            .range = {0.0f, 1.0f, 1.0f, 1.0f},  // stepped on/off, default on
        });
    }

    void prepare(const format::PrepareContext&) override {}

    void process(audio::BufferView<float>& output,
                 const audio::BufferView<const float>& input,
                 midi::MidiBuffer& midi_in,
                 midi::MidiBuffer& midi_out,
                 const format::ProcessContext&) override {
        // Pure MIDI effect: pass any audio through untouched.
        for (std::size_t ch = 0;
             ch < output.num_channels() && ch < input.num_channels(); ++ch) {
            auto in = input.channel(ch);
            auto out = output.channel(ch);
            for (std::size_t i = 0; i < out.size(); ++i) out[i] = in[i];
        }

        // Short messages always pass through.
        for (const auto& event : midi_in) midi_out.add(event);

        // SysEx is echoed only when enabled, payload + offset preserved.
        if (state().get_value(kEchoEnabled) >= 0.5f) {
            for (const auto& sx : midi_in.sysex()) {
                midi_out.add_sysex(
                    std::vector<uint8_t>(sx.data.begin(), sx.data.end()),
                    sx.sample_offset, sx.timestamp);
            }
        }
    }
};

inline std::unique_ptr<format::Processor> create_sysex_echo() {
    return std::make_unique<SysexEchoProcessor>();
}

} // namespace pulp::examples::classic

// Pulls in the inline definition of build_sysex_echo_editor (declared above) so create_view()
// links in the plugin adapter and the headless tests alike. After the class so
// the editor header sees a complete definition; its re-include is a no-op.
#include "sysex_echo_editor.hpp"
