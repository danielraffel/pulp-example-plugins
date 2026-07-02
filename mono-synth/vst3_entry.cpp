// MonoSynth VST3 entry point.
#include "mono_synth.hpp"
#include <pulp/format/vst3_entry.hpp>

// Unique, stable plugin ID — never change once shipped.
static const Steinberg::FUID MonoSynthUID(0x50554C50, 0x4D6F6E6F, 0x00000001,
                                          0x00000001);

PULP_VST3_PLUGIN(MonoSynthUID, "MonoSynth",
                 Steinberg::Vst::PlugType::kInstrumentSynth,
                 "Pulp Examples", "0.1.0",
                 "https://github.com/danielraffel/pulp",
                 pulp::examples::classic::create_mono_synth)
