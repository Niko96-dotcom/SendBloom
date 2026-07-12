---
phase: 16-engine-crossfade
verified: 2026-07-08T22:13:00Z
status: human_needed
score: 9/10 must-haves verified
behavior_unverified: 1
overrides_applied: 0
behavior_unverified_items:
  - truth: "Idle engine resets after fade completes; active engine tail is not cleared mid-fade"
    test: "Toggle authentic_color on/off in a DAW or test harness; capture reverb tail during and immediately after a 35 ms crossfade window"
    expected: "Outgoing engine reset() fires only once fade completes (mixGain no longer smoothing); incoming engine tail continues uninterrupted through the fade"
    why_human: "Reset timing is a post-fade state transition in SchroederTank32 — no unit or integration test asserts reset() call order vs mid-fade tail preservation"
human_verification:
  - test: "Toggle 32k Color during a held chord or sustained guitar input in a DAW at 48 kHz"
    expected: "No audible click above the automated max-adjacent-delta thresholds; crossfade feels smooth over ~35 ms"
    why_human: "Roadmap SC1 references audible click; automated tests proxy click via sample-delta metrics only"
---

# Phase 16: Engine Crossfade Verification Report

**Phase Goal:** Users can toggle 32k Color without clicks, NaN, or buffer overruns during performance
**Verified:** 2026-07-08T22:13:00Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Toggling `authentic_color` crossfades between engines over 20–50 ms without click above threshold (XFADE-01) | ✓ VERIFIED | `EngineCrossfade.h` clamps fade to 0.02–0.05 s (35 ms default); unit tests confirm 960–2400 samples at 48 kHz; click metrics `< 0.25f` / `< 0.5f` normalized; integration toggle `< 1.0f` — all `[XFADE-01]` tests pass |
| 2 | SchroederTank32 processes host and FixedRateAdapter blocks in parallel during fade with equal-power wet blend | ✓ VERIFIED | `SchroederTank32.h` lines 105–123: dual `processBlock` into scratch buffers + `engineCrossfade_.mixWetBlock`; `EngineCrossfade.h` uses sin/cos equal-power gains |
| 3 | GatedBloomChain uses block reverb path when `authentic_color` is on OR reverb reports crossfade active | ✓ VERIFIED | `GatedBloomChain.h` line 88: `if (! authenticColor && ! reverb->isCrossfading())` per-sample early path; else block wet-send + `reverb->processBlock` |
| 4 | PluginProcessor detects smoothed `authenticColorTarget` 0.5 crossings instead of block-start-only latch | ✓ VERIFIED | `PluginProcessor.cpp` lines 290–296: per-sample edge on `prevAuthenticSmoothed` vs `authenticColorTarget`; `lastAuthenticColorSmoothed_` persisted across blocks (lines 272, 321) |
| 5 | Idle engine resets after fade completes; active engine tail is not cleared mid-fade | ⚠️ PRESENT_BEHAVIOR_UNVERIFIED | `SchroederTank32.h` lines 125–131 call `hostEngine.reset()` or `fixedRate_.reset()` only when `! engineCrossfade_.isCrossfading()` after mix — present and wired, but no test exercises reset call order |
| 6 | 1000 explicit `authentic_color` toggles with rotating block sizes produce finite output (XFADE-02) | ✓ VERIFIED | `RealtimeStressTest.cpp` case `[XFADE-02]`: 1000 toggles, `kBlockSizes` rotation, `std::isfinite` on every sample — 602002 assertions pass |
| 7 | Peak sample magnitude stays below 4.0f across 1000-toggle stress | ✓ VERIFIED | Same test: `REQUIRE (peak < 4.0f)` passes |
| 8 | Per-block max adjacent delta stays below 1.0f integration click gate | ✓ VERIFIED | Same test: per-block channel-0 delta `< 1.0f` on all 1000 blocks |
| 9 | EngineCrossfade and SchroederTank32 processBlock bodies pass static heap-allocation scan | ✓ VERIFIED | `IntegrationAllocScanTest.cpp` includes `EngineCrossfade.h` (mixWetBlock extraction) + existing sources; 24 assertions pass |
| 10 | Blocks larger than prepared `maxBlockSize` do not overrun scratch during crossfade | ✓ VERIFIED | `SchroederTank32.h` and `GatedBloomChain.h` early-return when `numSamples > maxBlockSize_` before scratch access; stress test uses sizes ≤ 1024 with `prepareToPlay(48000, 1024)` |

**Score:** 9/10 truths verified (1 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | ----------- | ------ | ------- |
| `source/EngineCrossfade.h` | Equal-power wet mix helper and SmoothedValue ramp | ✓ VERIFIED | 72 lines; substantive implementation; used by SchroederTank32 |
| `source/SchroederTank32.h` | Dual-engine crossfade state machine | ✓ VERIFIED | Scratch buffers sized in `prepare`; crossfade branch wired |
| `source/GatedBloomChain.h` | Crossfade-aware block routing | ✓ VERIFIED | `requestEngineCrossfade` forwarder; `isCrossfading()` gate |
| `source/PluginProcessor.cpp` | Per-block edge detection and crossfade trigger | ✓ VERIFIED | `chain.requestEngineCrossfade` on 0.5 crossing |
| `tests/EngineCrossfadeTest.cpp` | Unit fade duration and click metric gates | ✓ VERIFIED | 4 test cases, all pass |
| `tests/RealtimeStressTest.cpp` | 1000-toggle `[XFADE-02]` case | ✓ VERIFIED | Test present and passing |
| `tests/IntegrationAllocScanTest.cpp` | EngineCrossfade.h in alloc scan | ✓ VERIFIED | 4-source kSources array includes EngineCrossfade.h |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | --- | --- | ------ | ------- |
| `source/PluginProcessor.cpp` | `source/GatedBloomChain.h` | `requestEngineCrossfade` on 0.5 edge | ✓ WIRED | Line 294: `chain.requestEngineCrossfade(authenticColorTarget > 0.5f)` |
| `source/GatedBloomChain.h` | `source/SchroederTank32.h` | Block path when `isCrossfading()` | ✓ WIRED | Line 88 gate + line 106 `reverb->processBlock` |
| `source/SchroederTank32.h` | `source/EngineCrossfade.h` | Parallel host + fixed blend | ✓ WIRED | Lines 108–123: dual engines → `mixWetBlock` |

*Note: `gsd-tools query verify.key-links` returned false positives (searched for `beginEngineCrossfade` per plan wording); manual grep confirms all three links wired.*

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| -------- | ------------- | ------ | ------------------ | ------ |
| `EngineCrossfade.h` | `mixGain` SmoothedValue | `prepare()` sample-rate ramp | Real per-sample blend weights | ✓ FLOWING |
| `SchroederTank32.h` | `hostCrossfadeScratch_` / `fixedCrossfadeScratch_` | Dual `processBlock` from live wet-send | Real engine outputs blended | ✓ FLOWING |
| `PluginProcessor.cpp` | `authenticColorTarget` | `SmoothedParameterBank` per sample | Drives edge detection | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------ |
| XFADE-01 unit + integration gates | `cd build && ./Tests "[XFADE-01]" -r compact` | 5 assertions in 4 cases, all pass | ✓ PASS |
| XFADE-02 1000-toggle stress | `cd build && ./Tests "[XFADE-02]" -r compact` | 602002 assertions, 1 case, pass | ✓ PASS |
| Static alloc scan | `cd build && ./Tests "integrated processBlock bodies have no heap allocation tokens" -r compact` | 24 assertions, pass | ✓ PASS |
| Phase 16 ctest slice | `cd build && ctest -R "EngineCrossfade\|XFADE\|authentic color toggling\|integrated processBlock bodies"` | 5/5 pass | ✓ PASS |

### Probe Execution

Step 7c: SKIPPED — no probe scripts declared for Phase 16.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ---------- | ----------- | ------ | -------- |
| XFADE-01 | 16-01-PLAN.md | Toggling `authentic_color` crossfades 20–50 ms without click above threshold | ✓ SATISFIED | EngineCrossfade helper + SchroederTank32 dual path + integration click gate |
| XFADE-02 | 16-02-PLAN.md | 1000-toggle stress: no NaN/Inf, no heap alloc, no buffer overrun | ✓ SATISFIED | RealtimeStressTest 1000-toggle case + IntegrationAllocScanTest extension |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| ---- | ---- | ------- | -------- | ------ |
| — | — | None | — | No TBD/FIXME/XXX/TODO/HACK/PLACEHOLDER in phase-modified source or test files |

### Human Verification Required

### 1. Post-fade idle engine reset timing

**Test:** Toggle `authentic_color` on/off while monitoring reverb tail continuity; optionally instrument or breakpoint `hostEngine.reset()` / `fixedRate_.reset()` in `SchroederTank32::processBlock`.
**Expected:** Reset fires only after `engineCrossfade_.isCrossfading()` becomes false; no tail truncation on the active engine during the fade window.
**Why human:** Code path is present but no automated test asserts reset call order vs mid-fade tail preservation.

### 2. Audible click quality in DAW (optional smoke)

**Test:** Toggle 32k Color during a held chord at 48 kHz in a DAW.
**Expected:** No audible click; crossfade feels smooth over ~35 ms.
**Why human:** Roadmap SC1 references audible perception; automated gates use max-adjacent-delta proxies only.

### Gaps Summary

No blocking implementation gaps. XFADE-01 and XFADE-02 requirements are satisfied in code with passing automated gates. One behavior-dependent invariant (post-fade idle engine reset) is present and wired but not exercised by a named test — routed to human verification. Phase may proceed to Phase 17 after human spot-check of reset timing (and optional DAW smoke).

---

_Verified: 2026-07-08T22:13:00Z_
_Verifier: Claude (gsd-verifier)_
