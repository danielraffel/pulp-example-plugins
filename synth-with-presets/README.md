# Synth With Presets

A monophonic instrument with a **factory preset bank**. A Program selector loads
one of three presets (Pluck / Pad / Sine) into the timbre parameters; the synth
responds to note-on/off, **pitch bend** (±2 semitones), and the **mod wheel**
(CC1 → vibrato). Builds on the minimal osc + ADSR voice.

## What it validates (Pulp SDK contract)

- The instrument contract (`PluginCategory::Instrument`, `accepts_midi`,
  MIDI → audio) with pitch bend and a continuous controller.
- A preset system: selecting a Program loads its values (clamped to ranges);
  the Program and all parameters round-trip through the host save/load path.
- The **classic recall trap, avoided**: custom plugin state records the active
  program only to keep `process()` from re-applying the factory preset over the
  user's edited values on restore — verified by a round-trip test that edits a
  parameter after selecting a program.

## Notes

Presets are param snapshots applied on Program change. Per-voice polyphony,
preset *naming*, and user banks are natural extensions left out to keep the
example focused on the recall semantics.
