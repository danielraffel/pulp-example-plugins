#pragma once

// gui-zoo — a UI-fixture plugin. It carries NO DSP: audio passes through
// untouched and it exposes no parameters. Its entire point is the EDITOR — a
// scrolling board of Pulp's widget primitives (buttons, knobs, faders, meters,
// text inputs, an XY pad, a dropdown, segmented controls, a channel strip, a
// MIDI keyboard, status/feedback surfaces) under the dark Ink & Signal theme,
// so the widget set can be browsed live in any AU / VST3 / CLAP host.
//
// Two rendering paths, one board:
//   * build_gui_zoo()      — the full, non-scrolling board the headless
//                            screenshot fixture renders against baseline.png.
//   * create_view()        — the same board wrapped in a ScrollView sized to a
//                            sane min viewport, so a host window smaller than
//                            the full board can still reach every widget.

#include <pulp/design/design_system.hpp>
#include <pulp/format/processor.hpp>
#include <pulp/view/view.hpp>
#include <pulp/view/widget_gallery.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>

namespace pulp::examples::guizoo {

/// Full, non-scrolling widget board under the dark (default) or light Ink &
/// Signal theme. The returned view sizes itself; read view->bounds() for the
/// canvas. Used by the headless fixture + screenshot baseline.
inline std::unique_ptr<pulp::view::View> build_gui_zoo(bool dark = true) {
    return pulp::view::build_widget_gallery(pulp::design::ink_signal_theme(dark));
}

using namespace pulp;

class GuiZooProcessor : public format::Processor {
public:
    // The point of this plugin: hand the host the scrolling widget board. The
    // host owns + resizes the returned tree; the ScrollView keeps the full
    // board reachable within whatever bounds the host gives it.
    std::unique_ptr<view::View> create_view() override {
        return view::build_scrolling_widget_gallery(
            design::ink_signal_theme(/*dark=*/true), 720.0f, 760.0f);
    }

    format::PluginDescriptor descriptor() const override {
        return {.name = "GuiZoo", .manufacturer = "Pulp Examples",
                .bundle_id = "com.pulp.examples.gui-zoo", .version = "0.1.0",
                .category = format::PluginCategory::Effect,
                .input_buses = {{"Audio In", 2}}, .output_buses = {{"Audio Out", 2}}};
    }

    void define_parameters(state::StateStore&) override {}   // UI fixture — no params
    void prepare(const format::PrepareContext&) override {}

    void process(audio::BufferView<float>& output,
                 const audio::BufferView<const float>& input,
                 midi::MidiBuffer&, midi::MidiBuffer&,
                 const format::ProcessContext&) override {
        // Pass audio through untouched; zero-fill any output channel with no
        // matching input so an extra output is never left uninitialized.
        const std::size_t ch = std::min(output.num_channels(), input.num_channels());
        for (std::size_t c = 0; c < ch; ++c) {
            auto in = input.channel(c);
            auto out = output.channel(c);
            const std::size_t n = std::min(in.size(), out.size());
            for (std::size_t i = 0; i < n; ++i) out[i] = in[i];
        }
        for (std::size_t c = ch; c < output.num_channels(); ++c) {
            auto out = output.channel(c);
            for (std::size_t i = 0; i < out.size(); ++i) out[i] = 0.0f;
        }
    }
};

inline std::unique_ptr<format::Processor> create_gui_zoo() {
    return std::make_unique<GuiZooProcessor>();
}

} // namespace pulp::examples::guizoo
