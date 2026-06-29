#pragma once
#include "state_memo.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_state_memo_editor(state::StateStore& store) {
    return build_effect_editor(store, EffectEditorSpec{
        .title = "STATE MEMO", .subtitle = "custom state + gain",
        .knobs = {{kGain, "Gain"}}, .bypass_id = 0, .has_bypass = false});
}
}
