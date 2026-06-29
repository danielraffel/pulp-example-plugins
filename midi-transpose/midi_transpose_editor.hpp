#pragma once
#include "midi_transpose.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_midi_transpose_editor(state::StateStore& store) {
    return build_effect_editor(store, EffectEditorSpec{
        .title = "MIDI TRANSPOSE", .subtitle = "semitone note shifter",
        .knobs = {{kSemitones, "Semitones"}}, .bypass_id = 0, .has_bypass = false});
}
}
