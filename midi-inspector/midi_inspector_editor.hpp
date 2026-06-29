#pragma once
#include "midi_inspector.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_midi_inspector_editor(state::StateStore& store) {
    // Per-type filters as toggles, plus a read-only recent-event log. The
    // processor publishes a live Snapshot (counts + ring + dropped) via a
    // TripleBuffer (see snapshot() in midi_inspector.hpp); create_view() only
    // receives the StateStore, so the panel shows a deterministic sample of the
    // log shape here. A host that wires the processor's snapshot() into the view
    // can refresh these rows per frame; the static fixture keeps the headless
    // screenshot baseline stable.
    return build_effect_editor(store, EffectEditorSpec{
        .title = "MIDI INSPECTOR", .subtitle = "event logger + filters",
        .controls = {{kLogNotes, "Notes", Control::Kind::Toggle, {}},
                     {kLogCC, "CC", Control::Kind::Toggle, {}},
                     {kLogOther, "Other", Control::Kind::Toggle, {}}},
        .status_line = "4 shown \xC2\xB7 0 dropped",
        .log_items = {"Note On    ch1  C3   vel 100",
                      "Note On    ch1  E3   vel  98",
                      "CC         ch1  #74  64",
                      "Pitch Bend ch1  +0"},
        .bypass_id = 0, .has_bypass = false});
}
}
