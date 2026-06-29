#pragma once
#include "mpe_spreader.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_mpe_spreader_editor(state::StateStore& store) {
    // No parameters — a title-only status panel (channel assignment is automatic).
    return build_effect_editor(store, EffectEditorSpec{
        .title = "MPE SPREADER", .subtitle = "per-note member channels",
        .knobs = {}, .bypass_id = 0, .has_bypass = false});
}
}
