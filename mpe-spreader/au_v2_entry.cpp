// MPE Spreader AU v2 entry point (aumi — kAudioUnitType_MIDIProcessor).
#include "mpe_spreader.hpp"
#include <pulp/format/au_v2_entry.hpp>

PULP_AU_MIDI_PLUGIN(MpeSpreaderAU, pulp::examples::classic::create_mpe_spreader)
