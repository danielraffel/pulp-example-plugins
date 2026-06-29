#pragma once
#include "sysex_echo.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_sysex_echo_editor(state::StateStore& store) {
    return build_effect_editor(store, EffectEditorSpec{
        .title = "SYSEX ECHO", .subtitle = "round-trips System Exclusive",
        .knobs = {{kEchoEnabled, "Echo"}}, .bypass_id = 0, .has_bypass = false});
}
}
