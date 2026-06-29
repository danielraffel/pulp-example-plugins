#pragma once
#include "gain.hpp"
#include "../ink_signal_editor.hpp"
#include <memory>
namespace pulp::examples::classic {
inline std::unique_ptr<view::View> build_gain_editor(state::StateStore& store) {
    // Fader exercises attach_fader for the linear gain; Pan stays a rotary knob.
    return build_effect_editor(store, EffectEditorSpec{
        .title = "GAIN", .subtitle = "gain + equal-power pan",
        .controls = {{kGainAmount, "Gain", Control::Kind::Fader, {}},
                     {kGainPan, "Pan", Control::Kind::Knob, {}}},
        .bypass_id = 0, .has_bypass = false});
}
}
