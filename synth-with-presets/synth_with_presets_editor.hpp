#pragma once
#include "synth_with_presets.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_synth_with_presets_editor(state::StateStore& store) {
    // Program combo is populated from the processor's own factory names.
    std::vector<std::string> programs;
    for (const char* name : SynthWithPresetsProcessor::kProgramNames)
        programs.emplace_back(name);
    return build_effect_editor(store, EffectEditorSpec{
        .title = "SYNTH PRESETS", .subtitle = "factory preset bank",
        .controls = {{kSpProgram, "Program", Control::Kind::Combo, programs},
                     {kSpWaveform, "Wave", Control::Kind::Combo,
                      {"Sine", "Saw", "Square", "Triangle"}},
                     {kSpAttack, "Attack", Control::Kind::Knob, {}},
                     {kSpDecay, "Decay", Control::Kind::Knob, {}},
                     {kSpSustain, "Sustain", Control::Kind::Knob, {}},
                     {kSpRelease, "Release", Control::Kind::Knob, {}}},
        .bypass_id = 0, .has_bypass = false});
}
}
