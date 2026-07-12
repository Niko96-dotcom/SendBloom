---
phase: 23-input-level-gate-truth
verified: 2026-07-12T16:55:00Z
status: passed
score: 3/3 must-haves verified
behavior_unverified: 0
overrides_applied: 0
re_verification: false
---

# Phase 23: Input, Level & Gate Truth Verification Report

**Phase Goal:** Input, Level, and Gate controls agree with their labels — clockwise Input drives wet harder, Level scales wet only, PostHard is brutal but de-clicked.

**Verified:** 2026-07-12T16:55:00Z  
**Status:** passed  
**Re-verification:** No — initial verification

**Success model:** `[input-anchors]` + `[posthard]` green; CORE-01…13 green; `[pressure-release]`, `[oversized-block]`, `[true-bypass]`, `[midi-apvts]` remain green; `[shipping-policy]` stays red.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | ------- | ---------- | -------------- |
| 1 | Input maps −9/0/+9 at 0/0.5/1 via one canonical curve shared by display and DSP; clockwise increases wet drive; dry tap before Input | ✓ VERIFIED | `inputGainDb = -9+18*smoothstep`; UI calls `ParameterCurves::inputGainDb`; `[input-anchors]` PASS; `[release][io][input_gain]` + DryPath PASS |
| 2 | Level changes wet return only; dead dry-gain fields/smoothers gone; Gate Sens advanced with existing ID + canonical dB display | ✓ VERIFIED | `levelEqualPower` dry=1; no `dryGain`/`levelDryGain` in `source/`; AdvancedDrawer → `inputThresholdDb`; routing/snapshot PASS |
| 3 | PostHard closes with 0.75 ms ramp (not snap), reaches zero within 1 ms, chops wet within 15 ms; PreSoft long close retained | ✓ VERIFIED | `[posthard]` PASS (7 asserts); `[gate][TEST-02]` PASS; `[gate][NoiseGate]` PreSoft soft-floor PASS |

**Score:** 3/3 truths verified

### Required Artifacts

| Artifact | Expected | Status |
| -------- | ----------- | ------- |
| `source/ParameterCurves.h` | ADR-V1-08/09 curves | ✓ VERIFIED |
| `source/PluginEditor.cpp` | Input display → `inputGainDb` | ✓ VERIFIED |
| `source/ParameterSnapshot.h` / `SmoothedParameterBank.h` | No dry Level state | ✓ VERIFIED |
| `source/ui/AdvancedDrawer.cpp` | Gate Sens dB | ✓ VERIFIED |
| `source/NoiseGate.h` | 0.75 ms PostHard ramp | ✓ VERIFIED |
| `tests/V1ContractInputAnchorsTest.cpp` | `[input-anchors]` green | ✓ VERIFIED |
| `tests/V1ContractPostHardRampTest.cpp` | `[posthard]` green | ✓ VERIFIED |
| `23-UI-SPEC.md` | Display-truth contract | ✓ VERIFIED |

### Key Link Verification

| From | To | Via | Status |
| ---- | --- | --- | ------ |
| Input knob | `inputGainDb` | formatter | ✓ WIRED |
| Gate Sens knob | `inputThresholdDb` | AdvancedDrawer formatter | ✓ WIRED |
| levelNorm | wetGain only | ParallelWetMixer | ✓ WIRED |
| PostHard close | linear ramp → 0 | 0.75 ms step | ✓ WIRED |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------- |
| Input anchors | `Builds/Tests "[input-anchors]"` | PASS | ✓ |
| PostHard ramp | `Builds/Tests "[posthard]"` | PASS | ✓ |
| Curves / snapshot | `[curves][parm]` / `[parm][snapshot]` | PASS | ✓ |
| Routing / Level | `[chain][routing]` | PASS | ✓ |
| Gate timing | `[gate][TEST-02]` / `[gate][NoiseGate]` | PASS | ✓ |
| Input wet-only | `[release][io][input_gain]` | PASS | ✓ |
| Pressure regression | `[pressure-release]` | PASS | ✓ |
| Oversized regression | `[oversized-block]` | PASS | ✓ |
| True bypass regression | `[true-bypass]` | PASS | ✓ |
| MIDI purity regression | `[midi-apvts]` | PASS | ✓ |
| Shipping (Phase 25) | `[shipping-policy]` | FAIL (exit 42) | ✓ still red |

### Anti-Goals

| Item | Status |
| ---- | ------ |
| Shipping-policy / branding | Left red for Phase 25 |
| Predelay / SRC / wet-dirt | Untouched (Phase 24) |

## Requirements Coverage

| ID | Status |
|----|--------|
| CORE-01…CORE-13 | ✓ |

## Verdict

**passed** — Phase 23 goals met; safe to run `gsd-tools query phase.complete 23` / `phase complete 23`.
