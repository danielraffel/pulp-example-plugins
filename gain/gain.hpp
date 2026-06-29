#pragma once

// Gain — a plain utility effect: linear output gain plus an equal-power pan.
//
// The set's missing archetype — a transparent audio effect with no MIDI and no
// timbre, just the two controls every channel strip starts with. Gain is a
// linear multiplier; Pan distributes the stereo signal across L/R with an
// equal-power (−3 dB center) law so the perceived loudness stays constant as
// the image moves. There is no creative DSP here — it is the textbook
// "multiply + balance" effect, kept deliberately small as a teaching example.

#include <pulp/format/processor.hpp>
#include <pulp/view/view.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>

namespace pulp::examples::classic {

enum GainParams : state::ParamID {
    kGainAmount = 1,  // 0..2 linear, default 1 (unity)
    kGainPan    = 2,  // -1 (hard left) .. +1 (hard right), default 0 (center)
};

// Defined out-of-line in gain_editor.hpp (included at the bottom of this file).
// Forward-declared so the editor the screenshot tests render is the same
// tree the host receives from create_view().
std::unique_ptr<view::View> build_gain_editor(state::StateStore& store);

class GainProcessor : public format::Processor {
public:
    // Hand the host our dark Ink & Signal editor; the framework owns the
    // returned tree and may call this once per attached editor window.
    std::unique_ptr<view::View> create_view() override { return build_gain_editor(state()); }

    format::PluginDescriptor descriptor() const override {
        return {.name = "Gain", .manufacturer = "Pulp Examples",
                .bundle_id = "com.pulp.examples.gain", .version = "0.1.0",
                .category = format::PluginCategory::Effect,
                .input_buses = {{"Audio In", 2}}, .output_buses = {{"Audio Out", 2}}};
    }

    void define_parameters(state::StateStore& store) override {
        store.add_parameter({.id = kGainAmount, .name = "Gain", .unit = "",
                             .range = {0.0f, 2.0f, 1.0f, 0.0f}});
        store.add_parameter({.id = kGainPan, .name = "Pan", .unit = "",
                             .range = {-1.0f, 1.0f, 0.0f, 0.0f}});
    }

    void prepare(const format::PrepareContext&) override {}

    void process(audio::BufferView<float>& output,
                 const audio::BufferView<const float>& input,
                 midi::MidiBuffer&, midi::MidiBuffer&,
                 const format::ProcessContext&) override {
        const float gain = std::clamp(state().get_value(kGainAmount), 0.0f, 2.0f);
        const float pan = std::clamp(state().get_value(kGainPan), -1.0f, 1.0f);

        // Equal-power pan: gl² + gr² == 1 for every pan position, so the summed
        // power is constant as the image sweeps. Center (pan 0) lands at the
        // −3 dB point (gl == gr == 0.707).
        const float angle = (pan + 1.0f) * 0.25f * 3.14159265358979323846f;  // 0..π/2
        const float gl = std::cos(angle);
        const float gr = std::sin(angle);

        const std::size_t channels =
            std::min(output.num_channels(), input.num_channels());
        for (std::size_t ch = 0; ch < channels; ++ch) {
            const float w = gain * (ch == 1 ? gr : gl);
            auto in = input.channel(ch);
            auto out = output.channel(ch);
            const std::size_t n = std::min(in.size(), out.size());
            for (std::size_t i = 0; i < n; ++i) out[i] = in[i] * w;
        }
        // Silence any output channels with no matching input.
        for (std::size_t ch = channels; ch < output.num_channels(); ++ch) {
            auto out = output.channel(ch);
            for (std::size_t i = 0; i < out.size(); ++i) out[i] = 0.0f;
        }
    }
};

inline std::unique_ptr<format::Processor> create_gain() {
    return std::make_unique<GainProcessor>();
}

} // namespace pulp::examples::classic

// Pulls in the inline definition of build_gain_editor (declared above) so create_view()
// links in the plugin adapter and the headless tests alike. After the class so
// the editor header sees a complete definition; its re-include is a no-op.
#include "gain_editor.hpp"
