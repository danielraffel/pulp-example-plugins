// WAMv2 entry point for the midi-inspector demo.
#include "midi_inspector.hpp"
#include <memory>
std::unique_ptr<pulp::format::Processor> pulp_wam_make_processor() {
    return pulp::examples::classic::create_midi_inspector();
}
