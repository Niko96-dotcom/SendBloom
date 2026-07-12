---
phase: 13-fixed-rate-adapter-r8brain
plan: 04
subsystem: audio-dsp
tags: [r8brain, ProperSRC, SchroederTankCore, Goertzel, SRC-06, realtime]

requires:
  - phase: 13-02
    provides: RateConverterPair upsample/downsample with FIFO
  - phase: 13-03
    provides: LegacyAccumulatorPath and FixedRateAdapter shell routing
provides:
  - Complete ProperSRC sandwich around SchroederTankCore@32768 Hz
  - SRC-06 14825 Hz imaging reduction gate vs LegacyAccumulator
  - TEST-10 reset parity for legacy and ProperSRC modes
  - Realtime stress and static heap-allocation scan for processBlock
affects: [14-gated-bloom-chain-integration]

tech-stack:
  added: []
  patterns:
    - "prepare-time double scratch sized via getMaxUpsampledLen; processBlock upsample→core loop→downsample"
    - "SRC-06 uses guitar-pluck HF fixture at 14825 Hz Goertzel (HF diagnostics contract)"

key-files:
  created: []
  modified:
    - source/FixedRateAdapter.h
    - tests/FixedRateAdapterTest.cpp

key-decisions:
  - "SRC-06 uses guitar-pluck input instead of impulse because legacy anti-image SVF masks 14825 Hz on impulse tails"
  - "ProperSRC drives SchroederTankCore at 32768 with host-path bright/dark coeffs per RESEARCH"

patterns-established:
  - "renderAdapterImpulse / renderAdapterInput helpers for offline adapter IR renders"

requirements-completed: [SRC-01, SRC-02, SRC-03, SRC-06, TEST-10]

coverage:
  - id: D1
    description: "ProperSRC upsample→core@32768→downsample sandwich with variable blocks at four host rates"
    requirement: SRC-01
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter ProperSRC impulse at 48 kHz produces finite bright tail"
        status: pass
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter ProperSRC impulse is finite at four host rates"
        status: pass
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter ProperSRC tolerates variable block sizes"
        status: pass
    human_judgment: false
  - id: D2
    description: "processBlock realtime-safe — no heap tokens; 10k random block stress"
    requirement: SRC-02
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter ProperSRC realtime stress with random block sizes"
        status: pass
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter processBlock has no heap allocation tokens"
        status: pass
    human_judgment: false
  - id: D3
    description: "Multi-rate and variable-block passthrough through adapter ProperSRC path"
    requirement: SRC-03
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter ProperSRC impulse is finite at four host rates"
        status: pass
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter ProperSRC tolerates variable block sizes"
        status: pass
    human_judgment: false
  - id: D4
    description: "ProperSRC reduces 14825 Hz tail imaging ≥70% vs LegacyAccumulator"
    requirement: SRC-06
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter ProperSRC reduces 14825 Hz imaging vs LegacyAccumulator"
        status: pass
    human_judgment: false
  - id: D5
    description: "reset() clears adapter state — impulse-after-reset matches fresh impulse"
    requirement: TEST-10
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter reset restores impulse parity for legacy and ProperSRC"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-07-08
status: complete
---

# Phase 13 Plan 04: ProperSRC Sandwich Summary

**r8brain upsample→SchroederTankCore@32768→downsample sandwich with SRC-06 70% imaging reduction at 14825 Hz**

## Performance

- **Duration:** ~25 min
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Replaced ProperSRC stub with full bandlimited sandwich using preallocated `internalScratch` / `internalProcessBuf`
- Added ProperSRC impulse, multi-rate, variable-block, reset parity, realtime stress, and static allocation tests
- SRC-06 passes: legacy 14825 Hz tail RMS ≈ 0.00144, proper ≈ 0.00037 (≤30% of legacy)
- Full ctest regression: 159/159 pass

## Task Commits

1. **Task 1: ProperSRC sandwich and full reset semantics** - `9bd9904` (feat)
2. **Task 2: SRC-06 imaging gate, reset parity, realtime stress** - `2745445` (test)

**Plan metadata:** skipped (commit_docs disabled)

## Files Created/Modified

- `source/FixedRateAdapter.h` — ProperSRC branch, prepare-time internal buffers
- `tests/FixedRateAdapterTest.cpp` — render helpers, SRC-06/TEST-10/realtime/static tests, routing test update

## Decisions Made

- SRC-06 uses guitar-pluck input (HighFrequencyRingingDiagnosticsTest fixture) because impulse legacy output has zero measurable 14825 Hz power after anti-image SVF masking

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Updated routing test for non-silent ProperSRC output**
- **Found during:** Task 1
- **Issue:** `FixedRateAdapter routes Authentic32Mode Off vs LegacyAccumulator` expected `properOut` RMS == 0 from stub
- **Fix:** Assert ProperSRC tail energy > 1e-4f alongside legacy
- **Files modified:** tests/FixedRateAdapterTest.cpp
- **Committed in:** `2745445`

**2. [Rule 2 - Missing Critical] SRC-06 fixture uses guitar pluck instead of impulse**
- **Found during:** Task 2
- **Issue:** Impulse legacy IR has zero Goertzel power at 14825 Hz (anti-image SVF fully masks); test could not satisfy `legacyPower > 0`
- **Fix:** Use guitar-pluck excitation per HF diagnostics contract; same tail window (19200, 2400) and 14825 Hz frequency
- **Files modified:** tests/FixedRateAdapterTest.cpp
- **Committed in:** `2745445`

**3. [Rule 3 - Blocking] Static test source path resolution from build dir**
- **Found during:** Task 2
- **Issue:** `source/FixedRateAdapter.h` not found when ctest cwd is `build/`
- **Fix:** `readAdapterHeaderSource()` tries `source/`, `../source/`, `../../source/`
- **Files modified:** tests/FixedRateAdapterTest.cpp
- **Committed in:** `2745445`

---

**Total deviations:** 3 auto-fixed (1 bug, 1 missing critical, 1 blocking)
**Impact on plan:** SRC-06 still validates ≥70% imaging reduction at 14825 Hz with established HF fixture; no GatedBloomChain wiring (Phase 14 scope preserved)

## Issues Encountered

None beyond deviations above

## User Setup Required

None

## Next Phase Readiness

- FixedRateAdapter ProperSRC path production-ready for Phase 14 GatedBloomChain integration
- `getRoundTripLatencySamples()` available for future PDC (Phase 17)

## Self-Check: PASSED

- FOUND: source/FixedRateAdapter.h
- FOUND: tests/FixedRateAdapterTest.cpp
- FOUND: 9bd9904
- FOUND: 2745445

---
*Phase: 13-fixed-rate-adapter-r8brain*
*Completed: 2026-07-08*
