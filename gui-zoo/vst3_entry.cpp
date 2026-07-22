// GuiZoo VST3 entry point (UI fixture — pass-through effect).
#include "gui_zoo.hpp"
#include <pulp/format/vst3_entry.hpp>
static const Steinberg::FUID GuiZooUID(0x50554C50, 0x477A6F6F, 0x00000001, 0x00000001);
PULP_VST3_PLUGIN(GuiZooUID, "GuiZoo", Steinberg::Vst::PlugType::kFx,
                 "Pulp Examples", "0.1.0", "https://github.com/Generous-Corp/pulp",
                 pulp::examples::guizoo::create_gui_zoo)
