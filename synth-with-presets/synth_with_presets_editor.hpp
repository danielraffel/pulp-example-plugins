#pragma once
#include "synth_with_presets.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_synth_with_presets_editor(state::StateStore& store) {
    return build_effect_editor(store, EffectEditorSpec{
        .title = "SYNTH PRESETS", .subtitle = "factory preset bank",
        .knobs = {{kSpProgram, "Program"}, {kSpWaveform, "Wave"},
                  {kSpAttack, "Attack"}, {kSpRelease, "Release"}},
        .bypass_id = 0, .has_bypass = false});
}
}
