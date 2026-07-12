---
phase: 12-schroeder-tank-core-extraction
plan: 01
subsystem: dsp
tags: [schroeder, reverb, catch2, juce, rt60, core-extraction]

requires: []
provides:
  - SchroederTankCore single-rate tank DSP class
  - Shared ReverbTestHelpers (measureRT60, maxAbsDiff, renderCoreImpulse)
  - Fixed 32768 Hz RT60 regression tests with CORE-01/CORE-02 gates
affects: [12-02, 12-03, 13-fixed-rate-adapter]

tech-stack:
  added: []
  patterns:
    - "Rate-parameterized SchroederTankCore with scaleDelay(rate/32768) only"
    - "Shared offline RT60 measurement in tests/ReverbTestHelpers.h"

key-files:
  created:
    - source/SchroederTankCore.h
    - tests/ReverbTestHelpers.h
    - tests/SchroederTankCoreTest.cpp
  modified: []

key-decisions:
  - "Core returns raw tank output * 0.85f without jlimit — clamping deferred to HostRateReverbEngine wrapper (plan 12-02)"
  - "CORE-02 validated behaviorally via RT60 at 32768 Hz plus constexpr table constant assertion"

patterns-established:
  - "SchroederTankCore: prepare(processingRate) + processSample(input, rt60, darkMix) — no authenticColor"
  - "Reverb test helpers in sendbloom::test::reverb namespace"

requirements-completed: [CORE-01, CORE-02, CORE-04]

coverage:
  - id: D1
    description: "SchroederTankCore single-rate API with no host/authentic branches"
    requirement: CORE-01
    verification:
      - kind: unit
        ref: "tests/SchroederTankCoreTest.cpp#SchroederTankCore has no host-path mode identifiers"
        status: pass
    human_judgment: false
  - id: D2
    description: "Fixed 32768 Hz core uses unscaled delay table"
    requirement: CORE-02
    verification:
      - kind: unit
        ref: "tests/SchroederTankCoreTest.cpp#SchroederTankCore uses unscaled delay table at 32768 Hz"
        status: pass
    human_judgment: false
  - id: D3
    description: "RT60 within ±15% at sizes 0.25, 0.5, 1.0 at 32768 Hz"
    requirement: CORE-04
    verification:
      - kind: unit
        ref: "ctest -R SchroederTankCore"
        status: pass
    human_judgment: false

duration: 35min
completed: 2026-07-08
status: complete
---

# Phase 12 Plan 01: SchroederTankCore Extraction Summary

**Single-rate SchroederTankCore extracted from host-path tank math with shared RT60 test helpers and fixed 32768 Hz impulse regression**

## Performance

- **Duration:** 35 min
- **Started:** 2026-07-08T19:37:00Z
- **Completed:** 2026-07-08T20:12:00Z
- **Tasks:** 3
- **Files modified:** 3 created

## Accomplishments
- `SchroederTankCore` exposes `prepare(processingRate)` and `processSample(input, rt60, darkMix)` with branch-free host-path DSP
- `tests/ReverbTestHelpers.h` centralizes `measureRT60`, `maxAbsDiff`, and `renderCoreImpulse`
- Five Catch2 tests pass: three RT60 cases at 32768 Hz, CORE-01 source scan, CORE-02 unscaled table check

## Task Commits

Each task was committed atomically:

1. **Task 1: Shared reverb test helpers** - `36f5409` (test)
2. **Task 2: SchroederTankCore single-rate extraction** - `0b550f5` (feat)
3. **Task 3: Fixed-rate RT60 tests and CORE-01 static gate** - `e1150c5` (test)

**Plan metadata:** skipped (commit_docs disabled)

## Files Created/Modified
- `source/SchroederTankCore.h` - Rate-parameterized Schroeder tank (predelay, APFs, combs, modulated tank AP)
- `tests/ReverbTestHelpers.h` - Shared offline RT60 measurement and IR rendering helpers
- `tests/SchroederTankCoreTest.cpp` - Fixed-rate RT60 and CORE-01/CORE-02 gate tests

## Decisions Made
- Core omits `jlimit(-4, 4)` output clamp — wrapper layer responsibility per RESEARCH.md Pattern 2
- CORE-02 delay-length proof uses RT60 behavioral validation plus `kSeriesApfDelays[0] == 167` constexpr check

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed missing namespace on delay table constant in test**
- **Found during:** Task 3 (build)
- **Issue:** `SchroederTank32DelayTable` used without `sendbloom::` qualifier caused compile error
- **Fix:** Qualified as `sendbloom::SchroederTank32DelayTable::kSeriesApfDelays[0]`
- **Files modified:** tests/SchroederTankCoreTest.cpp
- **Verification:** `cmake --build build --target Tests` succeeds
- **Committed in:** e1150c5

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Trivial compile fix; no scope change.

## Issues Encountered
- Fresh `build/` directory required full CMake configure (~3 min) before tests could run

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- `SchroederTankCore` ready for `HostRateReverbEngine` wrapper (plan 12-02)
- Fixed-rate adapter in Phase 13 can wrap core at 32768 Hz without double-scaling risk

## Self-Check: PASSED
- FOUND: source/SchroederTankCore.h
- FOUND: tests/ReverbTestHelpers.h
- FOUND: tests/SchroederTankCoreTest.cpp
- FOUND: 36f5409
- FOUND: 0b550f5
- FOUND: e1150c5

---
*Phase: 12-schroeder-tank-core-extraction*
*Completed: 2026-07-08*
