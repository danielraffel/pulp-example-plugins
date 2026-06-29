# Pulp Example Plugins

A small set of **companion example plugins** built as [Pulp](https://www.generouscorp.com/pulp/)
SDK examples — each a self-contained `Processor` that compiles to VST3 / CLAP /
standalone (and AU / AUv3 / AAX where the platform SDKs are available), with a
test that asserts real behavior.

Where [pulp-classic-effects](https://github.com/danielraffel/pulp-classic-effects)
showcases DSP effects, these round out the rest of the SDK contract surface — a
pure MIDI utility, a minimal instrument, and a UI fixture.

## Status

| Example | Editor | DSP | Test | Notes |
|---|---|---|---|---|
| MIDI Transpose | <img src="screenshots/midi-transpose.png" width="220"> | ✅ | ✅ | Pure MIDI effect — semitone note shifter, passes CC/bend/SysEx through |
| SysEx Echo | <img src="screenshots/sysex-echo.png" width="220"> | ✅ | ✅ | MIDI effect — round-trips System Exclusive payloads (echo on/off) |
| MIDI Inspector | <img src="screenshots/midi-inspector.png" width="220"> | ✅ | ✅ | MIDI pass-through that logs events (counts, ring, filters, dropped) via TripleBuffer |
| State Memo | <img src="screenshots/state-memo.png" width="220"> | ✅ | ✅ | Custom plugin state (a free-text memo) beyond automatable params; fail-safe (de)serialize |
| MPE Spreader | <img src="screenshots/mpe-spreader.png" width="220"> | ✅ | ✅ | MIDI effect — gives every held note its own MPE member channel (note-off integrity, recycle) |
| MonoSynth | <img src="screenshots/mono-synth.png" width="220"> | ✅ | ✅ | Minimal monophonic instrument (oscillator + ADSR), MIDI in → audio out |
| Synth With Presets | <img src="screenshots/synth-with-presets.png" width="220"> | ✅ | ✅ | Instrument + factory preset bank, pitch bend & mod-wheel vibrato; clean recall semantics |
| Gain | <img src="screenshots/gain.png" width="220"> | ✅ | ✅ | Plain utility effect — linear gain (fader) + equal-power pan (knob); stereo in → out |
| gui-zoo | <img src="gui-zoo/baseline.png" width="220"> | ✅ | ✅ | UI fixture — widgets/layout/Skia paint with a deterministic screenshot baseline |

## Credits

Inspired by truce-audio's [truce](https://github.com/truce-audio/truce) example
suite. Reimplemented from scratch on Pulp's own primitives.

## License

MIT — see [LICENSE](LICENSE). See also [Pulp licensing](https://www.generouscorp.com/pulp/licensing.html).

## Building

These examples consume the Pulp SDK. The simplest path is to scaffold against an
existing Pulp checkout/install with `pulp create`, which pins the SDK and wires
the build for you. To build this repo directly:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/path/to/pulp/install
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Editor screenshots

Each plugin exposes its editor through `Processor::create_view()`, so the dark
Ink & Signal panel shown below is what loads in any VST3 / AU / CLAP host or the
standalone app — no extra wiring per format.

The `Editor` column above is rendered from the baselines in `screenshots/`.
`test_editors.cpp` builds each editor through `create_view()` (the real host
path), re-renders it with Skia, and compares it pixel-wise against its committed
baseline, so an unintended UI change fails CI. For plugins with parameters it
also pushes every param off its default and asserts the render visibly changes,
proving the editor is bound to live plugin state. After a deliberate editor
change, rebake the baselines:

```bash
PULP_BAKE_SCREENSHOTS=1 ctest --test-dir build -R editors --output-on-failure
git add screenshots && git commit -m "chore: rebake editor screenshots"
```

The test skips cleanly when the SDK build has no Skia raster backend.
(gui-zoo keeps its own fixture test + `gui-zoo/baseline.png`.)
