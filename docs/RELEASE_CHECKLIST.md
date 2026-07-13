# SendBloom RC0 Release Checklist

**Milestone:** v1.0 RC0  
**Date:** 2026-07-12 (commercial-quality remediation pass)
**Scope:** Honest pre-tag gate — automated checks verified locally on macOS unless noted.

**Durable automated entry:** `bash scripts/verify-v1.sh` (defaults `BUILD_DIR=Builds`). Discovers ctest suite size at runtime, prints a truthful RED/GREEN status table, and labels human-only gates `human_needed`. Do **not** hard-code suite totals here or in scripts (BASE-06).

## Pre-Release Automated Gates (macOS local)

Historical results are not RC promotion evidence. Re-run via `scripts/verify-v1.sh` from a clean candidate build; record the runtime-discovered count, exact commit, and date in the release report.

- [x] Legal metadata audit passes (`bash scripts/check-legal-metadata.sh`) — also invoked by `verify-v1.sh`
- [x] Release AU + VST3 build — `Builds`, 2026-07-12 remediation worktree
- [x] Full runtime-discovered Catch2 suite — current suite passes with 1 explicit unavailable Zip API skip and 0 failures, 2026-07-13
- [x] pluginval strictness **10** on the fresh Release VST3 — final `SUCCESS`, 2026-07-12
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

## Fixed-Rate Reverb Truth (VERB-05)

SendBloom has one production reverb path at every host sample rate:

- **ProperSRC sandwich** per [ADR-003](architecture/ADR-003-proper-32k-src.md): host-rate block → r8brain conversion to **32,768 Hz** → `SchroederTankCore` at fixed delay-table lengths → r8brain conversion back to host rate (`FixedRateAdapter`).
- Per-comb RT60 feedback uses each comb's own delay reference.
- DARK remains the only production reverb-voicing switch, interpolating predelay and damping.
- Production damping and RT60 are smooth and are not quantized to speculative 9-bit steps.
- The fixed rate is an engineering approximation used for host-rate-independent behavior. It is **original software** — not a verified hardware sample rate, **not firmware-derived**, not bytecode, and not hardware cloning.

**Diagnostics only:** `Authentic32Mode::LegacyAccumulator` retains the old `processAuthentic` accumulator + host-rate anti-image SVF for A/B regression, and `HostRateReverbEngine` remains available for developer comparison. Neither is referenced by the production tank.

## RC1 Safety Freeze

- [x] `authentic_color` is absent from the APVTS parameter layout and generic host automation
- [x] All 8 factory presets omit `authentic_color`; old beta state entries are tolerated and discarded on load
- [x] **ProperSRC at 32,768 Hz** is the sole production reverb engine
- [x] Host-rate selection, engine crossfade, selector smoothing, and speculative 9-bit production quantization are absent
- ProperSRC remains covered by the Phase 18 acceptance gates (`bash scripts/enab-acceptance-gates.sh`)

## Multi-DAW Smoke (TEST-07) — human_needed

- [ ] **Logic** — `human_needed` — AU loads, UI renders, audio processes, presets accessible
- [ ] **Cubase** — `human_needed` — VST3 loads, plugin info correct, bypass works
- [ ] **REAPER** — `human_needed` — VST3 loads in FX chain, wet/dry and pressure pad functional

These require manual validation in each host before v1.0.0 (non-RC) tag. Never treat as automated PASS.

## Signing / Notarization / License — human_needed

- [ ] **Developer ID signing** — `human_needed`
- [ ] **Notarization / stapling** — `human_needed`
- [x] **JUCE path selected** — commercial path approved; see `docs/LICENSING_DECISION.md`
- [ ] **JUCE commercial entitlement fact** — `human_needed`; selection is not proof of coverage

## CI Matrix (GitHub Actions — not locally verified here)

CI workflow (`.github/workflows/build_and_test.yml`) runs per OS:

- Legal metadata audit
- Release build (AU + VST3 on macOS; VST3 on Windows/Linux)
- Catch2 via ctest (suite size discovered at runtime — not hard-coded in this checklist)
- pluginval strictness 10 on **VST3 only**

Confirm a green run ID/URL for the exact candidate commit on all three OS jobs before creating RC0. Local success cannot satisfy REL-08.

## Version Tag (only after every required gate is evidenced)

```bash
git tag -a v1.0.0-rc0 -m "SendBloom v1.0.0-rc0"
```

## Post-v1 Deferred

- CLAP / AAX formats
- Commercial storefront
- Dirt oversampling (not published as a shipping parameter)
- AU pluginval / automated AU smoke in CI
