# Web demos (WAMv2 / WebAssembly)

This directory cross-compiles the example plugins to **WAMv2** (Web Audio
Modules v2) — headless WebAssembly DSP modules that run in an `AudioWorklet` —
and the CI workflow (`.github/workflows/pages.yml`) publishes them to GitHub
Pages. No cross-origin isolation is required: the build is single-threaded, so
there is no `SharedArrayBuffer`, so no COOP/COEP headers are needed, which is
exactly why stock GitHub Pages (which cannot set custom headers) can host it.

`-sSINGLE_FILE` base64-embeds the wasm into the JS module, so there is not even
a separate `.wasm` fetch — the whole DSP is one `wam-dsp.js` ES module.

## What's here

- `CMakeLists.txt` — a **standalone** `emcmake` project (deliberately NOT wired
  into the repo-root `CMakeLists.txt`, which is the native plugin build). It
  `include()`s pulp's `tools/cmake/PulpWam.cmake` and declares one
  `SINGLE_FILE` WAM target per demoable plugin.
- `*_wasm.cpp` — one tiny entry file per plugin, each just `#include`s the
  plugin header and defines `pulp_wam_make_processor()`.
- `../scripts/build-web.sh` — configures, builds, and assembles the site tree
  (used by both local dev and CI).

## Build & serve locally

You need three things: Emscripten (pinned — see below), a pulp **source**
checkout, and choc headers.

```sh
# 1. Activate the pinned Emscripten toolchain.
source /path/to/emsdk/emsdk_env.sh   # emsdk must be installed at 6.0.2

# 2. Build + assemble docs/ (choc arg is the dir CONTAINING choc/).
PULP_ROOT=/path/to/pulp CHOC_INCLUDE=/path/to/choc ./scripts/build-web.sh

# 3. Serve it (module scripts require http://, not file://).
python3 -m http.server -d docs 8080
```

The script writes a static tree under `docs/` — one folder per demo, each with
`wam-dsp.js` (the SINGLE_FILE DSP module), `wam-processor.js`, and
`wam-runtime.mjs`, plus a shared `player/wam-plugin.js`. `docs/` is
**git-ignored** — no wasm or generated JS is ever committed.

## Toolchain pins — and why they are mandatory

**These pins are not theoretical; both failure modes were observed.**

- **Emscripten `6.0.2`.** emsdk `3.1.74`'s `-sSINGLE_FILE` glue decodes the
  embedded wasm with `atob()`, which **does not exist in
  `AudioWorkletGlobalScope`**. The worklet aborts on load and the plugin makes
  no sound — while the UI still looks fine, so it is a silent failure. emcc
  `6.0.2` emits its own base64 decoder and works. (pulp's own
  `web-plugins.yml` uses `version: latest` — a live trap; do not copy it.)
- **pulp ref pinned** (see `PULP_REF` in `pages.yml`). pulp's
  `wam-runtime.mjs` hardcodes an emscripten-version-specific `env` import-stub
  list; a mismatched emcc emits extra imports (e.g. `_emscripten_memcpy_js`
  under 3.1.74) that are absent from that list and instantiation fails. Pinning
  both the toolchain and the pulp ref keeps them in lockstep. The workflow also
  runs weekly so drift against the pin surfaces as a red build, not a dead site.

## Caveats (honest limitations)

- **gui-zoo is excluded.** It is a Processor whose only content is
  `create_view()`, and `create_view()` hard-returns `nullptr` in every WASM
  build (`core/format/src/wasm/headless_defaults.cpp`). With no DSP and no
  reachable UI on this path it is structurally undemoable, so it ships nothing.
- **The web editor is not the native editor.** Because `create_view()` returns
  `nullptr` in wasm, the on-page UI is a **token-faithful HTML/canvas
  recreation** of each plugin's controls, not the native Skia-rendered editor.
  It is driven by the plugin's parameter metadata over the worklet port.
- **Safari has no WebMIDI.** On Safari the on-screen keyboard is the primary
  (only) input for the instrument demos; Chrome/Edge additionally accept a
  connected MIDI controller.

## One-time repo setup (manual)

GitHub Pages must be switched to **"GitHub Actions"** as the publishing source:
repo **Settings → Pages → Build and deployment → Source → GitHub Actions**.
This is a one-time manual toggle — until it is flipped, the `deploy-pages` step
fails even when the build is green.
