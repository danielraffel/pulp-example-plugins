# State Memo

Demonstrates **custom plugin state that lives outside the parameter system**. The
plugin has one automatable parameter (Gain) handled by the usual `StateStore`
path, plus a free-text **memo** the user attaches to a session — not a parameter
(not automatable), persisted via `serialize_plugin_state()` /
`deserialize_plugin_state()`.

## What it validates (Pulp SDK contract)

- Plugin-owned state separate from parameters (`serialize_plugin_state` /
  `deserialize_plugin_state`), alongside a normal automatable parameter.
- **Fail-safe deserialization**: empty state keeps defaults; corrupt or
  truncated blobs are rejected without clobbering the current memo; a
  forward-version blob is read and its trailing future fields ignored.
- Parameter state round-trips through the host's save/load path.

## Format

```
[u32 version][u32 memo_len][memo bytes][... future fields, ignored ...]
```

`memo_len` is bounded against both the buffer and a 4 KB cap before any read, so
untrusted state can never over-read.
