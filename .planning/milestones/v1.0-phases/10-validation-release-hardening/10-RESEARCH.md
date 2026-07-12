# Phase 10: Validation & Release Hardening - Research

**Researched:** 2026-07-06
**Domain:** Catch2 traceability, pluginval 10, legal metadata, multi-DAW release gates
**Confidence:** HIGH

## Summary

Phase 10 is a **hardening-only** milestone gate — no new DSP features. Phases 1–9 already delivered **96/96 Catch2 tests** and **pluginval strictness 7** in CI. Research confirms **TEST-01 through TEST-05** are substantively covered by existing suites; the remaining work is **formal traceability**, **CI strictness 10**, **legal scan extension to presets/resources**, **clean-room documentation (LEG-02)**, **release checklist**, and **multi-DAW human smoke (TEST-07)**.

**Primary recommendation:** Three MVP plans — (1) requirements traceability + optional post-gate timing test, (2) legal hardening + pluginval 10 CI, (3) release checklist + multi-DAW docs + VERIFICATION.md + human checkpoint.

<user_constraints>
## User Constraints (from 10-CONTEXT.md)

### Locked Decisions

#### Testing
- Fill any test gaps for TEST-01 through TEST-07
- Full ctest suite must pass before phase close
- Raise CI STRICTNESS_LEVEL to 10

#### Legal
- Extend check-legal-metadata.sh to presets and resources
- Document clean-room positioning (LEG-01, LEG-02)
- No Rainger / Reverb-X / Igor in metadata, presets, or docs

#### Multi-DAW
- Document smoke procedures for Logic, Cubase, REAPER (human verify)
- Cannot automate DAW tests in CI

### Claude's Discretion
Release checklist format, any missing edge-case tests at planner discretion.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TEST-01 | Parameter curve unit tests | `ParameterCurvesTest.cpp`, `ParameterSnapshotTest.cpp`, SchroederTank32 RT60 tests |
| TEST-02 | Gate unit tests (pre hum, post chop, dry passes) | `NoiseGateTest.cpp`, `DryNeverGatedTest.cpp`, `PluginBasics` post-gate integration |
| TEST-03 | Dry-path clean at distn=1 | `DryPathIntegrityTest.cpp` THD + tap extract |
| TEST-04 | Pressure send trail (tank energy 500 ms) | `GatedBloomChainTest` send release preserves tank energy |
| TEST-05 | Realtime safety 10k blocks | `RealtimeStressTest.cpp` |
| TEST-06 | pluginval strictness 10 | Bump `STRICTNESS_LEVEL: 10` in workflow; local validate |
| TEST-07 | Logic, Cubase, REAPER load | README multi-DAW smoke + human checkpoint |
| LEG-01 | No banned trademark refs | Extend `check-legal-metadata.sh` to `resources/presets/*.xml` |
| LEG-02 | Clean-room positioning documented | New `docs/CLEAN_ROOM.md` |
</phase_requirements>

## Test Coverage Audit (96 tests, 2026-07-06)

| Req | Existing Evidence | Gap |
|-----|-------------------|-----|
| TEST-01 | `ParameterCurvesTest` (RT60, distn, send curves), `ParameterSnapshotTest`, `SchroederTank32Test` RT60 | None — add traceability doc |
| TEST-02 | `NoiseGateTest` PreSoft floor vs PostHard zero; `DryNeverGatedTest`; `PluginBasics` post gate chops wet | Optional: explicit ≤15 ms wet-drop timing assertion |
| TEST-03 | `DryPathIntegrityTest` at distn=1 | None |
| TEST-04 | `GatedBloomChainTest` tank energy at 500 ms post-send-release | None |
| TEST-05 | `RealtimeStressTest` 10k blocks, varying sizes | None |
| TEST-06 | CI at strictness 7 | Bump to 10 |
| TEST-07 | Phase 1–9 per-phase DAW smoke in README | Consolidate Logic/Cubase/REAPER Phase 10 checklist |

## CI Audit

**File:** `.github/workflows/build_and_test.yml`

| Item | Current | Target |
|------|---------|--------|
| STRICTNESS_LEVEL | 7 | **10** |
| Legal audit | `source`, `tests`, `README`, `.github` | Add `resources/presets`, `resources/` |
| Duplicate step | Legal audit runs twice (lines 81–82 and 96–97) | Remove duplicate |

## Legal Audit Gaps

`scripts/check-legal-metadata.sh` scans `source`, `tests`, `README.md`, `.github/workflows` but **not**:

- `resources/presets/*.xml` (8 factory presets — PRST-01)
- `resources/` binary assets if any text metadata

Preset names audited manually: Sparkle_Verb, Dry_Dub_Sends, Cut_Sample_Gate, Spacerock_Burn, Dark_Bloom, Firm_Pressure, Gated_Room, Hot_Clip — **no banned terms**.

**LEG-02:** No standalone clean-room doc exists. `PROJECT.md` and `.cursor/rules/gsd-project.md` state clean-room intent; need `docs/CLEAN_ROOM.md` for release traceability.

## pluginval Strictness 10

Tracktion pluginval levels 1–10; level 10 adds exhaustive state/randomized validation. Project already passes level 7 locally on Release VST3. Expect longer CI run; no code changes anticipated.

## Multi-DAW Smoke (TEST-07)

Cannot automate in CI. Per-phase README smoke exists; Phase 10 consolidates **Logic (AU)**, **Cubase (VST3)**, **REAPER (VST3)** load + process checklist. Status: `human_needed`.

## Release Artifacts Needed

1. `docs/CLEAN_ROOM.md` — LEG-02
2. `docs/RELEASE_CHECKLIST.md` — version, formats, test gates, legal, DAW smoke
3. `.planning/phases/10-validation-release-hardening/VERIFICATION.md` — automated + human status

## Anti-Patterns

- **Adding DSP features** — out of scope for Phase 10
- **Skipping preset XML in legal scan** — PRST-01 bundle is user-visible metadata
- **Blocking milestone on DAW smoke** — document as `human_needed`, proceed with automated gates

## Planner Recommendation

| Plan | Wave | Focus |
|------|------|-------|
| 10-01 | 1 | Requirements traceability test + post-gate timing assertion |
| 10-02 | 2 | Legal scan extension, CLEAN_ROOM.md, pluginval 10 CI |
| 10-03 | 3 | RELEASE_CHECKLIST, multi-DAW README, VERIFICATION, human checkpoint |
