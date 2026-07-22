// SynthWithPresets VST3 entry point.
#include "synth_with_presets.hpp"
#include <pulp/format/vst3_entry.hpp>

// Unique, stable plugin ID — never change once shipped.
static const Steinberg::FUID SynthWithPresetsUID(0x50554C50, 0x53507265,
                                                 0x00000001, 0x00000001);

PULP_VST3_PLUGIN(SynthWithPresetsUID, "SynthPresets",
                 Steinberg::Vst::PlugType::kInstrumentSynth,
                 "Pulp Examples", "0.1.0",
                 "https://github.com/Generous-Corp/pulp",
                 pulp::examples::classic::create_synth_with_presets)
