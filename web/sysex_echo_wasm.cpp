// WAMv2 entry point for the sysex-echo demo.
#include "sysex_echo.hpp"
#include <memory>
std::unique_ptr<pulp::format::Processor> pulp_wam_make_processor() {
    return pulp::examples::classic::create_sysex_echo();
}
