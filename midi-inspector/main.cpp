#include "midi_inspector.hpp"

#include <pulp/format/standalone.hpp>
#include <pulp/runtime/log.hpp>

int main()
{
    pulp::format::StandaloneApp app(pulp::examples::classic::create_midi_inspector);

    pulp::format::StandaloneConfig config;
    config.sample_rate = 48000.0;
    config.buffer_size = 256;
    config.input_channels = 0;
    config.output_channels = 2;
    config.supports_audio_input = false;
    app.set_config(config);

    if (!app.run_with_editor(false)) {
        pulp::runtime::log_error("MidiInspector: failed to run standalone editor");
        return 1;
    }
    return 0;
}
