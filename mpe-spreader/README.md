# MPE Spreader

Turns ordinary single-channel note input into **MPE** by giving every held note
its own member channel. Each note-on is assigned a free MPE Lower-Zone member
channel (MIDI channels 2..16); the matching note-off is routed back out on that
same channel, and the channel is recycled. Non-note messages pass through.

## What it validates (Pulp SDK contract)

- The MIDI-effect path (`accepts_midi` / `produces_midi`, `midi_in -> midi_out`).
- **Per-note channel assignment + note-off integrity** — the core MPE behavior
  that per-note pitch/pressure expression builds on.
- Headless tests using a `HeadlessHost`: simultaneous notes land on distinct
  channels; a note-off returns on its note's channel and frees it for reuse;
  velocity-0 note-on is treated as a note-off; the zone exhausts gracefully.

## Behavior

```
note-on   -> assign lowest free member channel (2..16), remember note->channel
note-off  -> emit on the remembered channel, free it (recycle)
other     -> passed through unchanged
```

Per-note expression (pitch bend / pressure routed to each note's channel) is a
natural extension left out to keep the example focused on channel assignment.
