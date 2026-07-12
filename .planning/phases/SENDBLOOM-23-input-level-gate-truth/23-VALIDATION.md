---
phase: 23
slug: input-level-gate-truth
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-07-12
---

# Phase 23 — Validation Strategy

> **Phase 23 success model:**
> - `[v1][contract][input-anchors]` and `[v1][contract][posthard]` flip **GREEN**
> - CORE-01…13 automated asserts green
> - Phase 20–22 greens stay green: `[pressure-release]`, `[oversized-block]`, `[true-bypass]`, `[midi-apvts]`
> - Later-phase red stays red: `[shipping-policy]`

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 + `Builds/Tests` |
| **Quick Input/Level** | `Builds/Tests "[input-anchors]" && Builds/Tests "[curves][parm]" && Builds/Tests "[parm][snapshot]"` |
| **Quick Gate** | `Builds/Tests "[posthard]" && Builds/Tests "[gate][NoiseGate]" && Builds/Tests "[gate][TEST-02]"` |
| **Regression** | `Builds/Tests "[pressure-release]" && Builds/Tests "[oversized-block]" && Builds/Tests "[true-bypass]" && Builds/Tests "[midi-apvts]"` |
| **Still-red** | `Builds/Tests "[v1][contract][shipping-policy]"` expect fail |
| **Full** | `ctest --test-dir Builds -C Release --output-on-failure` (not all-green until later phases) |

---

## Requirement → Automated Command Map

| Req ID | Behavior | Automated Command | Expect | Artifact |
|--------|----------|-------------------|--------|----------|
| CORE-01 | −9/0/+9 anchors | `Builds/Tests "[input-anchors]"` | pass | `V1ContractInputAnchorsTest` |
| CORE-02 | Display uses curve | source + editor formatter uses `inputGainDb` | pass | `PluginEditor.cpp` |
| CORE-03/04/05 | Drive / detector / dry tap | `[release][io][input_gain]` + DryPath | pass | ReleaseTruth / DryPath |
| CORE-06/07 | Gate Sens ID + dB display | AdvancedDrawer → `inputThresholdDb` | pass | `AdvancedDrawer.cpp` |
| CORE-08/09 | Wet-only Level; dead dry gone | `[chain][routing]` + no `levelDryGain` | pass | Snapshot/Bank/ParallelWet |
| CORE-10/11 | 0.75 ms ramp; ≤1 ms zero | `Builds/Tests "[posthard]"` | pass | `V1ContractPostHardRampTest` |
| CORE-12 | Chop ≤15 ms | `Builds/Tests "[gate][TEST-02]"` | pass | `PostGateTimingTest` |
| CORE-13 | PreSoft retained | `Builds/Tests "[gate][NoiseGate]"` PreSoft case | pass | `NoiseGateTest` |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Automated Command | Status |
|---------|------|------|-------------|-------------------|--------|
| 23-01-01 | 01 | 1 | CORE-01/02 | `[input-anchors]` + curves | pending |
| 23-01-02 | 01 | 1 | CORE-03/04/05 | ReleaseTruth input_gain + DryPath | pending |
| 23-02-01 | 02 | 1 | CORE-08/09 | routing + snapshot; no dryGain drain | pending |
| 23-02-02 | 02 | 1 | CORE-06/07 | Gate Sens dB formatter | pending |
| 23-03-01 | 03 | 2 | CORE-10/11 | `[posthard]` | pending |
| 23-03-02 | 03 | 2 | CORE-12/13 | PostGateTiming + PreSoft | pending |

---

## Regression Gates (must stay green)

- `[pressure-release]`
- `[oversized-block]`
- `[true-bypass]`
- `[midi-apvts]`

## Explicit non-goals this phase

- Do not green `[shipping-policy]`
- Do not change Predelay / ProperSRC / wet-dirt / branding
