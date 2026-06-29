# Gain

A plain **utility effect** — the channel-strip archetype the rest of this suite
was missing. Two controls, no MIDI, no timbre: a linear output **Gain** and an
equal-power **Pan**.

## What it validates (Pulp SDK contract)

- A transparent stereo audio effect (`PluginCategory::Effect`, stereo in →
  stereo out) with no MIDI.
- Two automatable parameters that round-trip through the host's save/load path.
- The editor exercises **`attach_fader`** (Gain) alongside a rotary knob (Pan),
  so the shared Ink & Signal builder is shown driving more than one widget kind.

## DSP

- **Gain** — a linear multiplier in `[0, 2]` (unity at 1.0).
- **Pan** — an equal-power (−3 dB center) law. With `angle = (pan + 1)·π/4`,
  the left/right weights are `cos(angle)` / `sin(angle)`, so `gl² + gr² == 1`
  for every pan position and center (pan 0) sits at the −3 dB point
  (`gl == gr ≈ 0.707`).

There is no creative DSP here — it is the textbook "multiply + balance" effect,
kept deliberately small as a teaching example.
