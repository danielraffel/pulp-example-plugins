// MPE Spreader VST3 entry point (MIDI effect: event in/out, no audio bus).
#include "mpe_spreader.hpp"
#include <pulp/format/vst3_entry.hpp>
static const Steinberg::FUID MpeSpreaderUID(0x50554C50, 0x4D706573, 0x00000001, 0x00000001);
PULP_VST3_PLUGIN(MpeSpreaderUID, "MpeSpreader", Steinberg::Vst::PlugType::kFx,
                 "Pulp Examples", "0.1.0", "https://github.com/Generous-Corp/pulp",
                 pulp::examples::classic::create_mpe_spreader)
