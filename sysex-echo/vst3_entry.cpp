// SysEx Echo VST3 entry point (MIDI effect: event in/out, no audio bus).
#include "sysex_echo.hpp"
#include <pulp/format/vst3_entry.hpp>
static const Steinberg::FUID SysexEchoUID(0x50554C50, 0x53797378, 0x00000001, 0x00000001);
PULP_VST3_PLUGIN(SysexEchoUID, "SysExEcho", Steinberg::Vst::PlugType::kFx,
                 "Pulp Examples", "0.1.0", "https://github.com/danielraffel/pulp",
                 pulp::examples::classic::create_sysex_echo)
