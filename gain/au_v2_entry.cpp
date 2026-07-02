// Gain AU v2 entry point (aufx — kAudioUnitType_Effect).
#include "gain.hpp"
#include <pulp/format/au_v2_entry.hpp>

PULP_AU_PLUGIN(GainAU, pulp::examples::classic::create_gain)
