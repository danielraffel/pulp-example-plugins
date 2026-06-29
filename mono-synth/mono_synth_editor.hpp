#pragma once
#include "mono_synth.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_mono_synth_editor(state::StateStore& store) {
    return build_effect_editor(store, EffectEditorSpec{
        .title = "MONOSYNTH", .subtitle = "oscillator + ADSR",
        .controls = {{kWaveform, "Wave", Control::Kind::Combo,
                      {"Sine", "Saw", "Square", "Triangle"}},
                     {kAttack, "Attack", Control::Kind::Knob, {}},
                     {kDecay, "Decay", Control::Kind::Knob, {}},
                     {kSustain, "Sustain", Control::Kind::Knob, {}},
                     {kRelease, "Release", Control::Kind::Knob, {}},
                     {kVolume, "Volume", Control::Kind::Knob, {}}},
        .bypass_id = 0, .has_bypass = false});
}
}
