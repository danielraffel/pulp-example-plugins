#pragma once
#include "mono_synth.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_mono_synth_editor(state::StateStore& store) {
    return build_effect_editor(store, EffectEditorSpec{
        .title = "MONOSYNTH", .subtitle = "oscillator + ADSR",
        .knobs = {{kWaveform, "Wave"}, {kAttack, "Attack"}, {kDecay, "Decay"},
                  {kSustain, "Sustain"}, {kRelease, "Release"}},
        .bypass_id = 0, .has_bypass = false});
}
}
