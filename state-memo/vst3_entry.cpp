// State Memo VST3 entry point.
#include "state_memo.hpp"
#include <pulp/format/vst3_entry.hpp>

// Unique, stable plugin ID — never change once shipped.
static const Steinberg::FUID StateMemoUID(0x50554C50, 0x536D656D, 0x00000001,
                                          0x0000000A);

PULP_VST3_PLUGIN(StateMemoUID, "StateMemo", Steinberg::Vst::PlugType::kFx,
                 "Pulp Examples", "0.1.0",
                 "https://github.com/danielraffel/pulp",
                 pulp::examples::classic::create_state_memo)
