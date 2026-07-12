---
phase: 22-midi-per-sample-control-delivery
verified: 2026-07-12T16:45:00Z
status: passed
score: 3/3 must-haves verified
behavior_unverified: 0
overrides_applied: 0
re_verification: false
---

# Phase 22: MIDI & Per-Sample Control Delivery Verification Report

**Phase Goal:** MIDI CC1 is sample-position-aware realtime pressure modulation (not APVTS mutation), and fast dynamic controls are consumed per sample.

**Verified:** 2026-07-12T16:45:00Z  
**Status:** passed  
**Re-verification:** No — initial verification

**Success model:** `[midi-apvts]` green; MIDI-01…10 + RT-04/06/07 green; `[pressure-release]`, `[oversized-block]`, `[true-bypass]` remain green.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | ------- | ---------- | -------------- |
| 1 | When connected, CC1 modulates effective pressure without `setValueNotifyingHost` or writing `send_amount` APVTS; saved state stays clean | ✓ VERIFIED | No `sendParam->store` in processBlock; `[midi-apvts]` PASS (5 asserts); MidiSendAmount / ReleaseTruth MIDI updated to purity |
| 2 | CC1 sample positions + multiple events applied in order; value 0 releases MIDI pressure; host/UI + MIDI via `max`; non-CC1 ignored | ✓ VERIFIED | `[v1][contract][midi]` PASS (18 asserts / 5 cases) — position gate, bounded window, non-CC1, max combine, finite @ 64/512/2048 |
| 3 | Span boundaries respect CC1; send/distn/threshold + input/level/output/bypass per-sample; finite/deterministic | ✓ VERIFIED | Span `min(..., nextCc1-offset)`; `distnScratch_`/`thresholdDbScratch_`/`sendGainScratch_` → chain arrays; `[per-sample]` PASS (18 asserts) |

**Score:** 3/3 truths verified

### Required Artifacts

| Artifact | Expected | Status |
| -------- | ----------- | ------- |
| `source/PluginProcessor.cpp` | MIDI cursor + no APVTS store + per-sample scratches | ✓ VERIFIED |
| `source/PluginProcessor.h` | applyCc1 / findNextCc1 + distn/threshold scratches | ✓ VERIFIED |
| `source/GatedBloomChain.h` | ADR-V1-06 array overload | ✓ VERIFIED |
| `tests/V1ContractMidiApvtsPurityTest.cpp` | `[midi-apvts]` green | ✓ VERIFIED |
| `tests/V1ContractMidiSampleAccurateTest.cpp` | sample-accurate MIDI contracts | ✓ VERIFIED |
| `tests/V1ContractPerSampleControlsTest.cpp` | RT-06/07 | ✓ VERIFIED |
| `tests/MidiSendAmountTest.cpp` | ADR-V1-03 updated | ✓ VERIFIED |

### Key Link Verification

| From | To | Via | Status |
| ---- | --- | --- | ------ |
| MidiBuffer CC1 | `setMidiPressureTarget` | connected-only at span offset | ✓ WIRED |
| Span loop | next CC1 | `findNextCc1SampleAfter` | ✓ WIRED |
| processSpan | chain arrays | distn/send/threshold `.data()` | ✓ WIRED |
| PressureController | send gain | `max(host, midi)` + smooth + curve | ✓ WIRED |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------- |
| MIDI purity | `Builds/Tests "[midi-apvts]"` | PASS | ✓ |
| MIDI send contracts | `Builds/Tests "[midi][send]"` | PASS | ✓ |
| Sample-accurate MIDI | `Builds/Tests "[v1][contract][midi]"` | PASS | ✓ |
| Per-sample controls | `Builds/Tests "[v1][contract][per-sample]"` | PASS | ✓ |
| Pressure regression | `Builds/Tests "[pressure-release]"` | PASS | ✓ |
| Oversized regression | `Builds/Tests "[oversized-block]"` | PASS | ✓ |
| True bypass regression | `Builds/Tests "[true-bypass]"` | PASS | ✓ |
| Chain suite | `Builds/Tests "[chain][GatedBloomChain]"` | PASS | ✓ |

### Anti-Goals (must stay red / untouched)

| Item | Status |
| ---- | ------ |
| PostHard / Input anchors / shipping-policy | Not fixed this phase (deferred) |

## Requirements Coverage

| ID | Status |
|----|--------|
| MIDI-01…MIDI-10 | ✓ |
| RT-04, RT-06, RT-07 | ✓ |

## Verdict

**passed** — Phase 22 goals met; safe to run `gsd-tools query phase.complete 22`.
