---
phase: 13-fixed-rate-adapter-r8brain
plan: 03
subsystem: dsp
tags: [legacy-accumulator, fixed-rate-adapter, authentic32, schroeder-tank, src-regression]

requires:
  - phase: 13-01
    provides: Authentic32Mode enum and SchroederTankCore reset
  - phase: 13-02
    provides: RateConverterPair prepare/reset/latency API
provides:
  - LegacyAccumulatorPath ported from SchroederTank32 processAuthentic for SRC-05 A/B
  - FixedRateAdapter shell routing Off/LegacyAccumulator with ProperSRC stub
affects:
  - 13-04 ProperSRC sandwich implementation
  - 14 GatedBloomChain facade wiring

tech-stack:
  added: []
  patterns: [header-only legacy reference path, mode-switch adapter shell without APVTS]

key-files:
  created: [source/LegacyAccumulatorPath.h, source/FixedRateAdapter.h]
  modified: [tests/FixedRateAdapterTest.cpp]

key-decisions:
  - "SRC-05 validation uses sample-for-sample parity with SchroederTank32 authentic path"
  - "Single-sample impulse is silent in reference authentic path; burst tail test proves non-zero energy"
  - "FixedRateAdapter processBlock rejects n > maxBlockSize; tests render in 512-sample blocks"

patterns-established:
  - "Legacy accumulator duplicated intentionally until Phase 14 facade wiring"
  - "ProperSRC stub returns silence until plan 13-04 implements r8brain sandwich"

requirements-completed: [SRC-04, SRC-05]

coverage:
  - id: D1
    description: "LegacyAccumulatorPath reproduces SchroederTank32 processAuthentic behavior"
    requirement: SRC-05
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#LegacyAccumulatorPath matches SchroederTank32 authentic impulse render"
        status: pass
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#LegacyAccumulatorPath burst input produces tank-matched tail energy"
        status: pass
    human_judgment: false
  - id: D2
    description: "FixedRateAdapter routes Off/LegacyAccumulator/ProperSRC stub without r8brain sandwich"
    requirement: SRC-04
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#FixedRateAdapter routes Authentic32Mode Off vs LegacyAccumulator"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-07-08
status: complete
---

# Phase 13 Plan 03: LegacyAccumulatorPath + FixedRateAdapter Shell Summary

**Legacy accumulator port with SchroederTank32 parity and FixedRateAdapter Off/Legacy routing shell for SRC-05 A/B baseline**

## Performance

- **Duration:** 12 min
- **Started:** 2026-07-08T20:25:31Z
- **Completed:** 2026-07-08T20:27:45Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Ported `LegacyAccumulatorPath` from `SchroederTank32::processAuthentic` (accumulator, hold, anti-image SVF, 9-bit quantize)
- Added `FixedRateAdapter` shell with `prepare`/`reset`/`processBlock` and `Authentic32Mode` switch
- Off mode outputs silence; LegacyAccumulator delegates to legacy path; ProperSRC stub returns zeros (13-04)
- SchroederTank32.h unchanged

## Task Commits

1. **Task 1: LegacyAccumulatorPath port from processAuthentic (SRC-05)** - `f1b4e3d` (feat)
2. **Task 2: FixedRateAdapter shell with mode routing (SRC-04)** - `5798334` (feat)

## Files Created/Modified
- `source/LegacyAccumulatorPath.h` - Standalone authentic accumulator reference path
- `source/FixedRateAdapter.h` - Mode switch shell with core/converters/legacy members
- `tests/FixedRateAdapterTest.cpp` - Parity, burst tail, and Authentic32Mode routing tests

## Decisions Made
- SRC-05 gate uses sample-for-sample parity with `SchroederTank32` authentic output (reference produces zero tail for single impulse)
- Added burst-input tail test (480 samples) to satisfy non-silence energy requirement aligned with reference behavior
- Adapter tests process in 512-sample blocks respecting `maxBlockSize` early-return guard

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Single impulse authentic path is silent in reference**
- **Found during:** Task 1 verification
- **Issue:** Plan test required max tail > 1e-4 for single impulse; `SchroederTank32` authentic also outputs zero
- **Fix:** Primary SRC-05 gate is parity with `SchroederTank32`; burst-input companion test proves non-zero tail energy
- **Files modified:** tests/FixedRateAdapterTest.cpp
- **Verification:** 4/4 LegacyAccumulator|Authentic32Mode ctest filters pass
- **Committed in:** 5798334

**2. [Rule 3 - Blocking] FixedRateAdapter test exceeded maxBlockSize**
- **Found during:** Task 2 verification
- **Issue:** `processBlock(n=8192)` with `maxBlockSize=512` early-returned without writing output
- **Fix:** Block-wise render loop capped at 512 samples per call
- **Files modified:** tests/FixedRateAdapterTest.cpp
- **Committed in:** 5798334

---

**Total deviations:** 2 auto-fixed (1 bug, 1 blocking)
**Impact on plan:** Test strategy adjusted to match reference DSP behavior; no API changes.

## Issues Encountered
None beyond authentic-path impulse silence discovery (resolved via parity + burst tests).

## User Setup Required
None.

## Next Phase Readiness
- Legacy path ready for SRC-06 Goertzel A/B vs ProperSRC in plan 13-04
- `FixedRateAdapter` core/converters prepared; ProperSRC stub awaits r8brain sandwich wiring

## Self-Check: PASSED
- FOUND: source/LegacyAccumulatorPath.h
- FOUND: source/FixedRateAdapter.h
- FOUND: tests/FixedRateAdapterTest.cpp
- FOUND: f1b4e3d
- FOUND: 5798334

---
*Phase: 13-fixed-rate-adapter-r8brain*
*Completed: 2026-07-08*
