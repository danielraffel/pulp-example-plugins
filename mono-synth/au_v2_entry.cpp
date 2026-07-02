// MonoSynth AU v2 entry point (aumu — kAudioUnitType_MusicDevice).
#include "mono_synth.hpp"
#include <pulp/format/au_v2_instrument_entry.hpp>

PULP_AU_INSTRUMENT(MonoSynthAU, pulp::examples::classic::create_mono_synth)
