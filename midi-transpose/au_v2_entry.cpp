// MidiTranspose AU v2 entry point (aumi — kAudioUnitType_MIDIProcessor).
#include "midi_transpose.hpp"
#include <pulp/format/au_v2_entry.hpp>

PULP_AU_MIDI_PLUGIN(MidiTransposeAU, pulp::examples::classic::create_midi_transpose)
