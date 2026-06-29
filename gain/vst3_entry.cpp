// Gain VST3 entry point.
#include "gain.hpp"
#include <pulp/format/vst3_entry.hpp>

// Unique, stable plugin ID — never change once shipped.
static const Steinberg::FUID GainUID(0x50554C50, 0x4761696E, 0x00000001,
                                     0x0000000B);

PULP_VST3_PLUGIN(GainUID, "Gain", Steinberg::Vst::PlugType::kFx,
                 "Pulp Examples", "0.1.0",
                 "https://github.com/danielraffel/pulp",
                 pulp::examples::classic::create_gain)
