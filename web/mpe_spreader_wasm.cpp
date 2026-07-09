// WAMv2 entry point for the mpe-spreader demo.
#include "mpe_spreader.hpp"
#include <memory>
std::unique_ptr<pulp::format::Processor> pulp_wam_make_processor() {
    return pulp::examples::classic::create_mpe_spreader();
}
