# Phase 23: Input, Level & Gate Truth - Research

**Researched:** 2026-07-12
**Domain:** Canonical Input curve (ADR-V1-08), unity-dry Level (ADR-V1-09), PostHard 0.75 ms de-click (ADR-V1-11), Gate Sens display truth
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

**CRITICAL:** Locked decisions are NON-NEGOTIABLE for planning/execution.

### Locked Decisions
- **D-01:** `inputGainDb = -9 + 18*smoothstep(norm)`; UI calls the same function; dry tap pre-Input
- **D-02:** Wet-only Level; remove dead dryGain smoother/fields; LEVEL=0 does not mute dry
- **D-03:** Keep `input_threshold` ID; Gate Sens advanced; display via `inputThresholdDb`
- **D-04:** PostHard 0.75 ms ramp; ≤1 ms to zero; 15 ms chop budget; PreSoft unchanged
- **D-05:** Flip `[input-anchors]` + `[posthard]`; preserve Phase 20–22 greens; leave shipping red

### Deferred (OUT OF SCOPE)
- Phase 24 DSP integrity, Phase 25 shipping/branding
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CORE-01 | Input −9/0/+9 at 0/0.5/1 | Fix `inputGainDb`; `[input-anchors]` |
| CORE-02 | Display calls canonical DSP curve | `PluginEditor` formatter → `inputGainDb` |
| CORE-03 | ↑ Input ↑ wet drive | Clockwise +9 dB; ReleaseTruth / drive spot-check |
| CORE-04 | ↑ Input ↑ detector at fixed raw | Detector after InputStage |
| CORE-05 | Dry tap before Input gain | Existing dry-path / ReleaseTruth |
| CORE-06 | Gate Sens advanced, existing ID | No ID rename; AdvancedDrawer only |
| CORE-07 | Gate Sens shows threshold dB | Formatter → `inputThresholdDb` |
| CORE-08 | Level changes wet return only | Already mix path; finalize curve API |
| CORE-09 | Remove dead dry-gain state | Snapshot + bank + processor drain |
| CORE-10 | PostHard 0.75 ms ramp not snap | Remove snap branch; linear/coeff ramp |
| CORE-11 | Reach zero ≤1 ms | Contract test already asserts |
| CORE-12 | Chop wet ≤15 ms silence | Keep `PostGateTimingTest` green |
| CORE-13 | PreSoft long close retained | Do not touch PreSoft timings |
</phase_requirements>

## Current State (Exact Bug Loci)

### 1. Input curve inverted/wrong — `ParameterCurves.h`
```cpp
// CURRENT (wrong): +9 → −3
return 9.0f + t * (-3.0f - 9.0f);
// REQUIRED (ADR-V1-08):
return -9.0f + 18.0f * smoothstep(norm);
```
`ParameterCurvesTest` still encodes the old +9/−3 anchors — must update.

### 2. Display arithmetic duplicate — `PluginEditor.cpp`
```cpp
const auto db = (0.5 - value) * 18.0; // clockwise decreases; not ParameterCurves
```
Must call `ParameterCurves::inputGainDb((float)value)` and format signed dB.

### 3. Dead dry Level state — Snapshot / Bank / Processor
- `levelEqualPower` still sets dual-sided dry/wet
- `dryGain` in snapshot; `levelDryGain` smoother; processor drains with `(void) getNextLevelDryGain()`
- Mix already uses `ParallelWetMixer::mix(dryTap, wet, wetGain)` only

### 4. Gate Sens display — `AdvancedDrawer.cpp`
Shows `String(value, 2)` on 0–1 norm. Need `inputThresholdDb`.

### 5. PostHard snap — `NoiseGate.h`
```cpp
if (floorGain <= 0.0f && ! isOpen) { gain = 0.0f; return gain; }
```
PostHard `releaseMs = 7.0f` is unused because snap wins. Replace with 0.75 ms deterministic ramp; attack stays 0.5 ms.

**Ramp shape (recommended):** linear decrement `1 / round(0.75e-3 * sr)` per sample while closed, clamp to 0. First step ≈ 1/36 @ 48 kHz ≈ 0.028 ≤ 0.05; reaches 0 by 0.75 ms (< 1 ms). Exponential with 0.75 ms τ fails CORE-11 (still ~0.26 at 1 ms).

## Recommended Implementation Shape

### Plan 01 — Input truth (CORE-01…05, CORE-02 UI)
1. Fix `inputGainDb`; update `ParameterCurvesTest`.
2. Wire Input formatter to `inputGainDb`.
3. Keep/extend drive+dry-tap proofs; flip `[input-anchors]`.

### Plan 02 — Level + Gate Sens (CORE-06…09)
1. `levelEqualPower`: `dry=1`, `wet=sin(halfPi*norm)` (or wet-only helper).
2. Remove `dryGain` / `levelDryGain` / drain.
3. Gate Sens formatter → `inputThresholdDb`.
4. Update snapshot / parallel-wet dual-scaled tests.

### Plan 03 — PostHard ramp (CORE-10…13)
1. Implement 0.75 ms close ramp; remove snap.
2. Flip `[posthard]`; keep `PostGateTimingTest` + PreSoft tests green.
3. Adjust `NoiseGateTest` cases that assumed instant zero on first closed sample.

## Validation Reality

| Tag | Expect after phase |
|-----|--------------------|
| `[input-anchors]` | GREEN |
| `[posthard]` | GREEN |
| `[pressure-release]` `[oversized-block]` `[true-bypass]` `[midi-apvts]` | stay GREEN |
| `[shipping-policy]` | stay RED |

## Confidence

HIGH — ADR text is explicit; failing contracts already encode success criteria; loci are single-file and well-bounded.
