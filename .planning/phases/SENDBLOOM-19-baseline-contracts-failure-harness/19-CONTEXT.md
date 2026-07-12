# Phase 19: Baseline, Contracts & Failure Harness - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning

<domain>
## Phase Boundary

Freeze current truth and install a durable failure harness before any production DSP/UI fixes: baseline report, requirement→phase→artifact traceability, Catch2 contract tests that fail for the intended defect reasons, a truthful `scripts/verify-v1.sh` automated gate runner, and honest `human_needed` marking for human-only gates. No production Pressure Mode, span, MIDI, Input/Level/Gate, reverb, or branding fixes in this phase.

</domain>

<decisions>
## Implementation Decisions

### Baseline Report & Traceability
- Baseline report lives at `.planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md` (phase-scoped, committed with the harness)
- BASE-01 captures commit SHA, branch, VERSION, build config, discovered Catch2/ctest count, CI workflow names, and known manual gaps
- Requirement→phase→artifact mapping verified via planning tables plus extendable `RequirementsTraceabilityTest` / test tags so each ID maps to a phase and at least one test tag or doc path
- Baseline is frozen before any production DSP/UI fix lands — Phase 19 commits only harness, docs, and intentionally failing contracts

### Contract Test Organization
- New failing contracts live in dedicated Catch2 files under `tests/` (e.g. `V1Contract*.cpp` or defect-named files), tagged `[v1][contract]` plus a stable defect id — do not weaken existing green tests to encode failures
- Defect coverage for Phase 19: pressure release flip, oversized-block dry fallback, true bypass unity, PostHard one-sample snap, Input display/DSP anchors, MIDI APVTS mutation from `processBlock`, banned shipping strings/filenames
- Each contract must fail for the intended reason (assert the broken contract), not flaky timing or unrelated setup
- Preexisting ProperSRC, HF, dry-integrity, and release-truth tests must remain green unless a requirement explicitly updates their contract (BASE-04)

### Verifier Script Design
- Add `scripts/verify-v1.sh` as the durable automated gate entry point (alongside existing `enab-acceptance-gates.sh` / `check-legal-metadata.sh`)
- Script discovers and runs the full automated gate set (configure/build/ctest + legal/release gates as applicable) and reports current red/green truthfully
- Do not hard-code expected total test counts in docs or scripts (BASE-06); discover counts at runtime
- Human-only gates are listed separately and never auto-pass

### Audio Metrics & Human Gates
- BASE-07 records baseline audio metrics for representative factory presets before DSP changes (document method + captured numbers/paths in the phase baseline artifact or adjacent metrics note)
- Human-only gates (AU pluginval, Windows/Linux matrix when not run, DAW smoke, signing/notarization, etc.) are marked `human_needed` in verifier/checklist output — never silently treated as pass (BASE-08)
- Reuse patterns from `docs/RELEASE_CHECKLIST.md` honesty model (verified vs not verified)
- Metrics capture may be offline/scripted analysis of factory preset renders; exact tooling at implementer discretion within “representative presets before DSP change”

### Claude's Discretion
- Exact Catch2 file naming, helper fixtures, and how baseline metrics are computed/stored (as long as committed and reproducible)
- Whether to wrap existing ENAB gates inside `verify-v1.sh` or invoke them as subprocesses
- Incremental vs single-PR layout of contract files within Phase 19 plans

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `tests/` — 43 Catch2 sources including `PressureSendTest`, `MidiSendAmountTest`, `BypassCrossfadeTest`, `PostGateTimingTest`, `DryPathIntegrityTest`, `ReleaseTruthTest`, `RequirementsTraceabilityTest`, ProperSRC/HF diagnostics
- `scripts/enab-acceptance-gates.sh` — ctest-filtered ENAB-01 gates + ADR-003 presence check
- `scripts/check-legal-metadata.sh` — legal/metadata scan
- `docs/RELEASE_CHECKLIST.md` — honest verified vs not-verified RC0 checklist (includes 113/113 note — must not hard-code going forward)
- `docs/DAW_SMOKE_RC0.md`, `docs/CLEAN_ROOM.md` — human/release surfaces
- `.github/workflows/build_and_test.yml` — Linux/macOS/Windows matrix, ctest, pluginval strictness 10

### Established Patterns
- Catch2 + ctest via CMake `Builds/` (or `BUILD_DIR`); tags like `[send]`, `[midi]`, `[traceability]`, `[regression]`
- APVTS mutation visible in existing MIDI tests (`MidiSendAmountTest` currently expects CC1 to update `send_amount` via APVTS — v1 contract will assert the opposite purity rule)
- Parallel dry/wet topology; `PressureSend`, `NoiseGate` PostHard, `BypassCrossfade` already unit-tested for current (pre-fix) behavior

### Integration Points
- New tests must be added to the CMake test target like existing `tests/*.cpp`
- `scripts/verify-v1.sh` should be the Phase 19/27 automated entry; CI may later call it but Phase 19 only needs the script + truthful local reporting
- Planning artifacts under `.planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/`

</code_context>

<specifics>
## Specific Ideas

User accepted all smart-discuss recommendations end-to-end for autonomous milestone run — prefer recommended defaults, minimize further grey-area pauses unless blockers/verification gates require a decision.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope. Production DSP/UI fixes deferred to Phases 20–25; RC packaging to Phase 27.

</deferred>
