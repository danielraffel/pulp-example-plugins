// SysEx Echo AU v2 entry point (aumi — kAudioUnitType_MIDIProcessor).
#include "sysex_echo.hpp"
#include <pulp/format/au_v2_entry.hpp>

PULP_AU_MIDI_PLUGIN(SysexEchoAU, pulp::examples::classic::create_sysex_echo)
