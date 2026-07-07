# SendBloom RC0 Release Checklist

**Milestone:** v1.0 RC0  
**Date:** 2026-07-07  
**Scope:** Honest pre-tag gate — automated checks verified locally on macOS unless noted.

## Pre-Release Automated Gates (macOS local — verified 2026-07-07)

- [x] Legal metadata audit passes (`bash scripts/check-legal-metadata.sh`)
- [x] Release AU + VST3 build succeeds (`cmake --build Builds --config Release`)
- [x] Full Catch2 suite passes (`ctest --test-dir Builds -C Release --output-on-failure`) — **113/113** (macOS local, 2026-07-07)
- [x] pluginval strictness **10** passes on Release **VST3** (macOS local)
- [x] Clean-room positioning documented (`docs/CLEAN_ROOM.md`)
- [x] `tests/ReleaseTruthTest.cpp` tracked and included in test target

## Not Verified Locally (honest gaps)

- [ ] **AU pluginval** — CI does not run pluginval on `.component`; requires manual AU validation or future CI step
- [ ] **Windows CI matrix** — not run on this machine
- [ ] **Linux CI matrix** — not run on this machine

## Artifacts

| Format | Path (local Release build) |
|--------|----------------------------|
| VST3 | `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3` |
| AU (macOS) | `Builds/SendBloom_artefacts/Release/AU/SendBloom.component` |

## Legal & Metadata (LEG-01, LEG-02)

- [x] No Rainger / Reverb-X / Igor in product name, CMake metadata, sources, presets, or README
- [x] Factory preset XML included in legal scan (`resources/presets/*.xml`)
- [x] Clean-room statement published (`docs/CLEAN_ROOM.md`)
- [x] No EEPROM, bytecode, firmware, or proprietary reverse-engineering claims in product-facing docs

## 32k Color Truth (VERB-05)

When **32k Color** (`authentic_color`) is enabled:

- Tank DSP is stepped at **32,768 Hz** and resampled to the host rate (`processAuthentic` accumulator).
- Delay lines use the fixed 32,768 Hz table lengths (not host-rate scaled).
- Per-comb RT60 feedback uses each comb's own delay reference.
- Damping and RT60 may be quantized to 9-bit steps.
- This is **original software** — not firmware-derived, not bytecode, not hardware cloning.

## Multi-DAW Smoke (TEST-07) — Human Required, Not Verified

- [ ] **Logic** — AU loads, UI renders, audio processes, presets accessible
- [ ] **Cubase** — VST3 loads, plugin info correct, bypass works
- [ ] **REAPER** — VST3 loads in FX chain, wet/dry and pressure pad functional

These require manual validation in each host before v1.0.0 (non-RC) tag.

## CI Matrix (GitHub Actions — not locally verified here)

CI workflow (`.github/workflows/build_and_test.yml`) runs per OS:

- Legal metadata audit
- Release build (AU + VST3 on macOS; VST3 on Windows/Linux)
- Catch2 via ctest
- pluginval strictness 10 on **VST3 only**

Confirm green CI on `main` before promoting RC0 → v1.0.0.

## Version Tag (on human approval after DAW smoke)

```bash
git tag -a v1.0.0-rc0 -m "SendBloom v1.0 RC0 — gated dirty ambience AU/VST3"
```

## Post-v1 Deferred

- CLAP / AAX formats
- Commercial storefront
- Extended stereo / dirt oversampling (shown disabled in UI)
- AU pluginval / automated AU smoke in CI
