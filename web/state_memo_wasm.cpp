// WAMv2 entry point for the state-memo demo.
#include "state_memo.hpp"
#include <memory>
std::unique_ptr<pulp::format::Processor> pulp_wam_make_processor() {
    return pulp::examples::classic::create_state_memo();
}
