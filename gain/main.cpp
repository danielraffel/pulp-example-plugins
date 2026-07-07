#include "gain.hpp"

#include <pulp/format/standalone.hpp>
#include <pulp/runtime/log.hpp>

int main()
{
    pulp::format::StandaloneApp app(pulp::examples::classic::create_gain);

    pulp::format::StandaloneConfig config;
    config.sample_rate = 48000.0;
    config.buffer_size = 256;
    config.input_channels = 2;
    config.output_channels = 2;
    config.supports_audio_input = true;
    app.set_config(config);

    if (!app.run_with_editor(false)) {
        pulp::runtime::log_error("Gain: failed to run standalone editor");
        return 1;
    }
    return 0;
}
