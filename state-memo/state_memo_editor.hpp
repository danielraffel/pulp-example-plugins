#pragma once
#include "state_memo.hpp"
#include "../ink_signal_editor.hpp"
#include <functional>
#include <memory>
#include <string>
namespace pulp::examples::classic {
// `get_memo`/`set_memo` reach the plugin's non-parameter memo string (the
// StateStore only holds numeric params). The processor passes lambdas bound to
// its own set_memo()/memo(); it owns the returned view, so the captured target
// outlives the TextEditor that calls these back.
inline std::unique_ptr<view::View> build_state_memo_editor(
    state::StateStore& store,
    std::function<std::string()> get_memo,
    std::function<void(const std::string&)> set_memo) {
    return build_effect_editor(store, EffectEditorSpec{
        .title = "STATE MEMO", .subtitle = "custom state + gain",
        .knobs = {{kGain, "Gain"}}, .bypass_id = 0, .has_bypass = false,
        .text_get = std::move(get_memo), .text_set = std::move(set_memo),
        .text_caption = "Session memo (saved with the plugin, not automatable)",
        .text_placeholder = "Type a note to store with this session…"});
}
}
