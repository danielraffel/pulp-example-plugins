// SynthWithPresets AU v2 entry point (aumu — kAudioUnitType_MusicDevice).
#include "synth_with_presets.hpp"
#include <pulp/format/au_v2_instrument_entry.hpp>

PULP_AU_INSTRUMENT(SynthWithPresetsAU, pulp::examples::classic::create_synth_with_presets)
