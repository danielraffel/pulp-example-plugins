# SysEx Echo

A pure MIDI effect that **round-trips System Exclusive** messages. Short MIDI
messages always pass through; when *Echo SysEx* is on, every incoming F0..F7
payload is copied to the output with its sample offset preserved.

## What it validates (Pulp SDK contract)

- The MIDI-effect path (`accepts_midi` / `produces_midi`, `midi_in -> midi_out`).
- Pulp's **SysEx contract**: variable-length payloads survive the buffer,
  including long payloads that exercise the adapter event pool.
- Headless tests using `pulp/format/validation_assertions.hpp` that assert a
  512-byte SysEx round-trips byte-for-byte (offset preserved), and that
  disabling echo drops SysEx while short messages still pass.

## Behavior

```
short messages:  always copied midi_in -> midi_out
SysEx (F0..F7):  copied when "Echo SysEx" is on, dropped when off
```
