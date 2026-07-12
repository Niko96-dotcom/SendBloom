---
phase: 13-fixed-rate-adapter-r8brain
plan: 02
subsystem: dsp
tags: [r8brain, src, rate-converter, catch2, realtime]

requires:
  - phase: 13-01
    provides: r8brain CPM pin and SchroederTankCore reset
provides:
  - RateConverterPair hostRate ↔ 32768 Hz r8brain wrapper with FIFO
  - Multi-rate and variable-block converter smoke tests
affects:
  - 13-03 FixedRateAdapter SRC sandwich
  - 13-04 SRC regression and imaging gates

tech-stack:
  added: []
  patterns: [prepare-time CDSPResampler allocation, preallocated leftover FIFO, getInLenBeforeOutPos latency query]

key-files:
  created: [source/RateConverterPair.h, tests/FixedRateAdapterTest.cpp]
  modified: []

key-decisions:
  - "Round-trip latency uses getInLenBeforeOutPos(0) sum because CDSPResampler::getLatency() always returns 0"
  - "leftoverDown_ FIFO preallocated in prepare; no heap in upsample/downsample paths"

patterns-established:
  - "RateConverterPair isolates r8brain lifecycle from FixedRateAdapter tank sandwich"
  - "Converter-only tests in FixedRateAdapterTest.cpp before adapter wiring in 13-03"

requirements-completed: [SRC-01, SRC-02, SRC-03]

coverage:
  - id: D1
    description: "RateConverterPair upsamples host float to double at 32768 Hz and downsamples with leftover FIFO"
    requirement: SRC-01
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#RateConverterPair passthrough is finite at multiple host rates"
        status: pass
    human_judgment: false
  - id: D2
    description: "CDSPResampler and scratch buffers allocated only in prepare(); process paths avoid heap"
    requirement: SRC-02
    verification:
      - kind: other
        ref: "source/RateConverterPair.h — no resize/new in upsample/downsample/reset"
        status: pass
    human_judgment: false
  - id: D3
    description: "Multi-rate prepare smoke at 44.1/48/88.2/96 kHz and variable blocks 1..512"
    requirement: SRC-03
    verification:
      - kind: unit
        ref: "ctest -R RateConverterPair --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "Round-trip latency queryable and stable after reset"
    requirement: SRC-01
    verification:
      - kind: unit
        ref: "tests/FixedRateAdapterTest.cpp#RateConverterPair round-trip latency is stable after reset"
        status: pass
    human_judgment: false

duration: 18min
completed: 2026-07-08
status: complete
---

# Phase 13 Plan 02: RateConverterPair Summary

**r8brain hostRate ↔ 32,768 Hz wrapper with prepare-time allocation, downsampler FIFO, and multi-rate smoke tests**

## Performance

- **Duration:** 18 min
- **Started:** 2026-07-08T20:15:00Z
- **Completed:** 2026-07-08T20:33:00Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Created header-only `RateConverterPair` with upsample/downsample block APIs and `reset()`
- Preallocated `CDSPResampler` instances and double scratch/FIFO buffers in `prepare()` only
- Added Catch2 multi-rate (44.1/48/88.2/96 kHz) and variable-block (1/7/64/512) smoke tests
- Exposed `getRoundTripLatencySamples()` via r8brain priming delay for Phase 17 PDC

## Task Commits

1. **Task 1: RateConverterPair r8brain wrapper** - `431623b` (feat)
2. **Task 2: Converter multi-rate smoke tests** - `75d7bdf` (test)

## Files Created/Modified
- `source/RateConverterPair.h` - r8brain up/down wrapper with FIFO and latency query (SRC-01, SRC-02)
- `tests/FixedRateAdapterTest.cpp` - converter-only multi-rate and variable-block smoke tests (SRC-03 partial)

## Decisions Made
- `getRoundTripLatencySamples()` sums `getInLenBeforeOutPos(0)` on both resamplers — `CDSPResampler::getLatency()` and `getLatencyFrac()` return zero on the master class
- Added `getMaxUpsampledLen()` helper for test buffer sizing (not in plan API but required by smoke harness)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] CDSPResampler getLatency returns zero**
- **Found during:** Task 2 (latency test)
- **Issue:** Plan specified `getLatency()` sum; master `CDSPResampler` always returns 0, failing `latencyBefore > 0`
- **Fix:** Use `getInLenBeforeOutPos(0)` sum for queryable priming delay (5160 at 48 kHz)
- **Files modified:** source/RateConverterPair.h
- **Verification:** Latency test passes; value stable after reset and re-prepare
- **Committed in:** 75d7bdf

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Latency API adjusted for r8brain master-class behavior; semantics preserved for Phase 17 PDC planning.

## Issues Encountered
- Working tree briefly contained out-of-scope 13-03 test cases from parallel work; restored converter-only scope per plan

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- `RateConverterPair` ready for `FixedRateAdapter` SRC sandwich in 13-03
- Multi-rate smoke baseline established for SRC-06 imaging regression in 13-04

## Self-Check: PASSED
- FOUND: source/RateConverterPair.h
- FOUND: tests/FixedRateAdapterTest.cpp
- FOUND: 431623b
- FOUND: 75d7bdf

---
*Phase: 13-fixed-rate-adapter-r8brain*
*Completed: 2026-07-08*
