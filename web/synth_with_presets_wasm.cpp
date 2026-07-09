// WAMv2 entry point for the synth-with-presets demo.
#include "synth_with_presets.hpp"
#include <memory>
std::unique_ptr<pulp::format::Processor> pulp_wam_make_processor() {
    return pulp::examples::classic::create_synth_with_presets();
}
