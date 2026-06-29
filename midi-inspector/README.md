# MIDI Inspector

A transparent MIDI **pass-through that logs what flows through**: per-type
counts, a ring of the most recent events for display, a dropped count for blocks
that overflow the ring, and per-type filters (notes / CC / other).

## What it validates (Pulp SDK contract)

- The MIDI-effect path (`accepts_midi` / `produces_midi`, faithful pass-through
  with sample offsets preserved).
- **Lock-free audio→UI publication**: the counts + ring are touched only on the
  audio thread; each block publishes an immutable `Snapshot` via
  `pulp::runtime::TripleBuffer`, and the UI thread reads the latest snapshot —
  no data race (the SDK's prescribed latest-value-publication pattern).
- An event queue (ring) ordered oldest→newest that wraps across blocks, type
  filters that gate display without affecting counts, and a dropped count.

## Note on the tests

The introspection tests read plugin state through `HeadlessHost::processor_as<T>()`
to call `snapshot()`. That typed accessor ships with the same SDK update as this
example; the pass-through test does not need it.

## Behavior

```
every message -> forwarded unchanged (midi_in -> midi_out)
counts        -> incremented per type (unfiltered)
display ring  -> most recent 64 filtered-in events, oldest -> newest
dropped       -> filtered-in events in a block beyond the ring capacity
```
