#include "midi_transpose.hpp"
#include <pulp/format/vst3_entry.hpp>
static const Steinberg::FUID MidiTransposeUID(0x50554C50, 0x4D746E73, 0x00000001, 0x00000001);
PULP_VST3_PLUGIN(MidiTransposeUID, "MidiTranspose", Steinberg::Vst::PlugType::kFx,
                 "Pulp Examples", "0.1.0", "https://github.com/danielraffel/pulp",
                 pulp::examples::classic::create_midi_transpose)
