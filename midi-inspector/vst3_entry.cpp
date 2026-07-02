// MIDI Inspector VST3 entry point (MIDI effect: event in/out, no audio bus).
#include "midi_inspector.hpp"
#include <pulp/format/vst3_entry.hpp>
static const Steinberg::FUID MidiInspectorUID(0x50554C50, 0x4D496E73, 0x00000001, 0x00000001);
PULP_VST3_PLUGIN(MidiInspectorUID, "MidiInspector", Steinberg::Vst::PlugType::kFx,
                 "Pulp Examples", "0.1.0", "https://github.com/danielraffel/pulp",
                 pulp::examples::classic::create_midi_inspector)
