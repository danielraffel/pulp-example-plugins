// WAMv2 entry point for the gain demo.
#include "gain.hpp"
#include <memory>
std::unique_ptr<pulp::format::Processor> pulp_wam_make_processor() {
    return pulp::examples::classic::create_gain();
}
