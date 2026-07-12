# SendBloom RC0 Release Checklist

**Milestone:** v1.0 RC0  
**Date:** 2026-07-07 (honesty model updated 2026-07-12 for Phase 19)  
**Scope:** Honest pre-tag gate — automated checks verified locally on macOS unless noted.

**Durable automated entry:** `bash scripts/verify-v1.sh` (defaults `BUILD_DIR=Builds`). Discovers ctest suite size at runtime, prints a truthful RED/GREEN status table, and labels human-only gates `human_needed`. Do **not** hard-code suite totals here or in scripts (BASE-06).

## Pre-Release Automated Gates (macOS local)

Historical note: legal/build/pluginval items below were verified locally on 2026-07-07. Re-run via `scripts/verify-v1.sh` before promoting a tag; record the **runtime-discovered** ctest count + date when re-verified.

- [x] Legal metadata audit passes (`bash scripts/check-legal-metadata.sh`) — also invoked by `verify-v1.sh`
- [x] Release AU + VST3 build succeeds (`cmake --build Builds --config Release`)
- [ ] Full Catch2 suite via ctest / `verify-v1.sh` — **do not claim a fixed N/N total**. Run `ctest --test-dir Builds -N` (or `verify-v1.sh`) and record the discovered count + verification date when green. While Phase 19 `[v1][contract]` failures remain, overall ctest / verify-v1 is expected **RED** — report truthfully; do not flip this box until contracts are fixed in later phases.
- [x] pluginval strictness **10** passes on Release **VST3** (macOS local, 2026-07-07). Optional re-check: `RUN_PLUGINVAL=1 PLUGINVAL_BIN=… bash scripts/verify-v1.sh`
- [x] Clean-room positioning documented (`docs/CLEAN_ROOM.md`)
- [x] `tests/ReleaseTruthTest.cpp` tracked and included in test target
- [x] ENAB-01 ProperSRC/HF gates (`BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh`) — also invoked by `verify-v1.sh`
- [x] Reference protocol/tooling and single-status claim invariant (`bash scripts/verify-reference-claims.sh`) — status is `original-inspired`

## Not Verified Locally (honest gaps) — human_needed

- [ ] **AU pluginval / auval** — `human_needed` (CI does not run pluginval on `.component`; not auto-PASS)
- [ ] **Windows CI matrix** — `human_needed` when not executed in this run
- [ ] **Linux CI matrix** — `human_needed` when not executed in this run
- [ ] **Hardware reference grids** — `human_needed`; no hardware captures supplied for Phase 26 closeout
- [ ] **Blind or level-matched listening review** — `human_needed`; no dated Niko verdict supplied

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

- **ProperSRC sandwich** per [ADR-003](architecture/ADR-003-proper-32k-src.md): host-rate block → r8brain upsample to **32,768 Hz** → `SchroederTankCore` at fixed delay-table lengths → r8brain downsample to host rate (`FixedRateAdapter`, `Authentic32Mode::ProperSRC`).
- Per-comb RT60 feedback uses each comb's own delay reference.
- Bright authentic damping uses `kAuthenticBrightDampingHz` (darker than host-rate bright).
- Damping and RT60 may be quantized to 9-bit steps.
- This is **original software** — not firmware-derived, not bytecode, not hardware cloning.

**Legacy diagnostics only:** `Authentic32Mode::LegacyAccumulator` retains the old `processAuthentic` accumulator + host-rate anti-image SVF (`kAuthenticAntiImageLpHz`) for A/B regression — not the production path.

## RC1 Safety Freeze

- [x] Fresh plugin load ships with `authentic_color` off (APVTS default `false`)
- [x] All 8 factory presets ship with `authentic_color=0`
- [x] **Host-rate Schroeder tank** is the production default for RC1
- ProperSRC validated via Phase 18 acceptance gates (`bash scripts/enab-acceptance-gates.sh`); **32k Color remains off by default** until product explicitly enables default-on
- When **32k Color** is enabled, the ProperSRC path described in **32k Color Truth (VERB-05)** above applies — not production-default until default-on is explicitly approved

## Multi-DAW Smoke (TEST-07) — human_needed

- [ ] **Logic** — `human_needed` — AU loads, UI renders, audio processes, presets accessible
- [ ] **Cubase** — `human_needed` — VST3 loads, plugin info correct, bypass works
- [ ] **REAPER** — `human_needed` — VST3 loads in FX chain, wet/dry and pressure pad functional

These require manual validation in each host before v1.0.0 (non-RC) tag. Never treat as automated PASS.

## Signing / Notarization / License — human_needed

- [ ] **Developer ID signing** — `human_needed`
- [ ] **Notarization / stapling** — `human_needed`
- [ ] **JUCE license decision** — `human_needed`

## CI Matrix (GitHub Actions — not locally verified here)

CI workflow (`.github/workflows/build_and_test.yml`) runs per OS:

- Legal metadata audit
- Release build (AU + VST3 on macOS; VST3 on Windows/Linux)
- Catch2 via ctest (suite size discovered at runtime — not hard-coded in this checklist)
- pluginval strictness 10 on **VST3 only**

Confirm green CI on `main` before promoting RC0 → v1.0.0. Local operators should prefer `bash scripts/verify-v1.sh` for the same automated surface.

## Version Tag (on human approval after DAW smoke)

```bash
git tag -a v1.0.0-rc0 -m "SendBloom v1.0 RC0 — gated dirty ambience AU/VST3"
```

## Post-v1 Deferred

- CLAP / AAX formats
- Commercial storefront
- Extended stereo / dirt oversampling (shown disabled in UI)
- AU pluginval / automated AU smoke in CI
