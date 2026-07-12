# Phase 23: Input, Level & Gate Truth - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-accepted smart-discuss recommendations (full autonomous run)

<domain>
## Phase Boundary

Input, Level, and Gate controls agree with their labels — clockwise Input drives wet harder (−9/0/+9 dB), Level scales wet return only, PostHard is brutal but de-clicked (0.75 ms ramp). Covers CORE-01…13. ADRs: ADR-V1-08, ADR-V1-09, ADR-V1-11.

Does NOT fix Predelay/mod/SRC/dirt (Phase 24) or shipping-policy/branding (Phase 25). Does NOT regress Phase 20–22 greens: `[pressure-release]`, `[oversized-block]`, `[true-bypass]`, `[midi-apvts]`.

</domain>

<decisions>
## Implementation Decisions (LOCKED — auto-accepted)

### D-01 — Canonical Input curve (ADR-V1-08 / CORE-01…05)
- Replace `ParameterCurves::inputGainDb` with `inputGainDb(norm) = -9.0f + 18.0f * smoothstep(norm)`
- Anchors: `0 → −9`, `0.5 → 0`, `1 → +9` within 1e-4
- UI Input formatter MUST call `ParameterCurves::inputGainDb` (no duplicated arithmetic) — flip display/DSP agreement (CORE-02)
- Clockwise increases wet-path drive and detector level (detector remains after InputStage)
- Dry tap remains before Input gain (existing ReleaseTruth / DryPath contracts)

### D-02 — Unity dry Level (ADR-V1-09 / CORE-08/09)
- `wetGain = sin(halfPi * levelNorm)`; dry contribution is always unity (`dryGain = 1` conceptually)
- Remove dead `dryGain` fields, `levelDryGain` smoother, and `(void) getNextLevelDryGain()` drain
- Update `levelEqualPower` (or replace with wet-only helper) so callers cannot reintroduce dual-sided dry scaling
- `LEVEL=0` means no wet return; dry stays audible
- Update tests that still assert dual-sided equal-power (`ParameterSnapshotTest`, `ParameterCurvesTest`, dual-scaled ParallelWet helper)

### D-03 — Gate Sens advanced display (CORE-06/07)
- Keep existing parameter ID `input_threshold` (no rename)
- Gate Sens remains advanced-drawer only
- Display reports canonical threshold via `ParameterCurves::inputThresholdDb(norm)` (dB string)

### D-04 — PostHard 0.75 ms de-click (ADR-V1-11 / CORE-10…13)
- Remove PostHard one-sample snap-to-zero path in `NoiseGate`
- Close uses a deterministic **0.75 ms** ramp to zero; attack remains **0.5 ms**
- After close command, gain reaches ≤ 1e-4 within **1.0 ms**
- Post gate still chops wet within **15 ms** after silence (`PostGateTimingTest` / CORE-12)
- PreSoft keeps long unobtrusive close (~150 ms) — do not change PreSoft character (CORE-13)
- No new user-facing PostHard timing control

### D-05 — Contract flips + regressions
- Flip green: `[input-anchors]`, `[posthard]`
- Preserve green: `[pressure-release]`, `[oversized-block]`, `[true-bypass]`, `[midi-apvts]`
- Leave red: `[shipping-policy]` (Phase 25)
- Update legacy tests that encode the old Input +9…−3 curve or instant PostHard snap

### Claude's Discretion
- Exact PostHard ramp implementation (linear sample ramp vs coeff) as long as 0.75 ms / ≤1 ms / |Δ|≤0.05 contracts hold
- Whether `levelEqualPower` keeps a dry out-param fixed at 1.0 or is replaced by `levelWetGain(norm)` — prefer minimal API churn
- How many additional CORE drive/detector behavioral tests beyond existing ReleaseTruth / PluginBasics coverage

</decisions>

<code_context>
## Existing Code Insights

### Exact defect loci
- `ParameterCurves::inputGainDb` — `9.0f + t * (-3-9)` → +9…−3 (inverted vs ADR-V1-08)
- `PluginEditor::formatSignedDbFromNorm` — `(0.5 - value) * 18` duplicates wrong arithmetic (CORE-02)
- `AdvancedDrawer` Gate Sens formatter shows raw 0–1, not threshold dB (CORE-07)
- `ParameterSnapshot` / `SmoothedParameterBank` still carry `dryGain` / `levelDryGain` though mix path uses wet-only `ParallelWetMixer` (CORE-09)
- `NoiseGate` PostHard: `if (floorGain <= 0 && !isOpen) gain = 0` one-sample snap (CORE-10)

### Reusable assets
- Failing contracts: `V1ContractInputAnchorsTest`, `V1ContractPostHardRampTest`
- Green proofs to preserve: ReleaseTruth `input_gain colors wet…`, PluginBasics `level scales wet return…`, `PostGateTimingTest`, `DryNeverGatedTest`, Phase 20–22 tags
- `ParallelWetMixer` already wet-only in DSP path

### Integration Points
- `source/ParameterCurves.h`, `ParameterSnapshot.h`, `SmoothedParameterBank.h`
- `source/NoiseGate.h`
- `source/PluginEditor.cpp`, `source/ui/AdvancedDrawer.cpp`
- `source/PluginProcessor.cpp` (dry-gain drain removal only)
- Tests under `tests/ParameterCurvesTest`, `ParameterSnapshotTest`, `NoiseGateTest`, `ParallelWetMixerTest`, V1 contracts

</code_context>

<specifics>
## Specific Ideas

Autonomous run — accept recommended defaults; pause only on blockers.
Write minimal `23-UI-SPEC.md` for Input/Level/Gate **display truth** only (not redesign).
Preserve `[pressure-release]`, `[oversized-block]`, `[true-bypass]`, `[midi-apvts]`.
Leave `[shipping-policy]` red.

</specifics>

<deferred>
## Deferred Ideas

- Predelay / dark tap / mod / ProperSRC / wet-dirt → Phase 24
- Shipping brand strings / faceplate / UX copy → Phase 25
- Reference capture / fidelity status → Phase 26

</deferred>
