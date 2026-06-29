#pragma once

// gui-zoo — the full Pulp widget gallery, themed and screenshotted headlessly.
//
// Rather than hand-roll a handful of widgets, this delegates to the SDK's own
// pulp::view::build_widget_gallery(theme), which lays out the entire widget set
// (buttons, sliders, knobs, toggles, meters, text, lists, ...) as a multi-
// section board via Yoga flex. Passing a light or dark Ink & Signal theme shows
// the same gallery under both palettes — the theming / diff-color showcase. The
// gallery sizes itself (query bounds() after building); render_to_png with the
// Skia backend produces the deterministic artifact the test floors + compares.

#include <pulp/design/design_system.hpp>
#include <pulp/view/view.hpp>
#include <pulp/view/widget_gallery.hpp>

#include <memory>

namespace pulp::examples::guizoo {

/// Build the full widget gallery under the dark (default) or light Ink & Signal
/// theme. The returned view sizes itself; read view->bounds() for the canvas.
inline std::unique_ptr<pulp::view::View> build_gui_zoo(bool dark = true) {
    return pulp::view::build_widget_gallery(pulp::design::ink_signal_theme(dark));
}

} // namespace pulp::examples::guizoo
