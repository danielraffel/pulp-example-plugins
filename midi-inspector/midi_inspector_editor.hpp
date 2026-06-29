#pragma once
#include "midi_inspector.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_midi_inspector_editor(state::StateStore& store) {
    return build_effect_editor(store, EffectEditorSpec{
        .title = "MIDI INSPECTOR", .subtitle = "event logger + filters",
        .knobs = {{kLogNotes, "Notes"}, {kLogCC, "CC"}, {kLogOther, "Other"}},
        .bypass_id = 0, .has_bypass = false});
}
}
