---
phase: 13-fixed-rate-adapter-r8brain
plan: 01
subsystem: dsp
tags: [r8brain, cmake, cpm, schroeder-tank, authentic32, reset, catch2]

requires:
  - phase: 12-core-extraction
    provides: SchroederTankCore single-rate tank at 32768 Hz
provides:
  - r8brain-free-src 7.1 CPM pin via INTERFACE CMake target
  - Authentic32Mode diagnostics enum (Off/LegacyAccumulator/ProperSRC)
  - SchroederTankCore::reset() for impulse parity and future adapter reset
affects:
  - 13-02 RateConverterPair
  - 13-03 FixedRateAdapter
  - 13-04 SRC regression tests

tech-stack:
  added: [r8brain-free-src 7.1 @ e71c31bf]
  patterns: [CPM DOWNLOAD_ONLY header fetch, INTERFACE CMake target, TDD reset parity test]

key-files:
  created: [cmake/R8brain.cmake, source/Authentic32Mode.h]
  modified: [CMakeLists.txt, source/SchroederTankCore.h, tests/SchroederTankCoreTest.cpp]

key-decisions:
  - "r8brain committed in cmake submodule (38bc0f5); parent repo wires include(R8brain) + SharedCode link"
  - "reset() uses existing atom reset() APIs without re-prepare or delay-length changes"

patterns-established:
  - "Third-party DSP headers via CPM DOWNLOAD_ONLY + INTERFACE target linked to SharedCode"
  - "Diagnostics enums live in standalone headers, never APVTS"

requirements-completed: [SRC-02, SRC-04, TEST-10]

coverage:
  - id: D1
    description: "r8brain-free-src 7.1 fetched via CPM and linked to SharedCode as INTERFACE target"
    requirement: SRC-02
    verification:
      - kind: other
        ref: "cmake -B build && test -f build/_deps/r8brain-free-src-src/CDSPResampler.h"
        status: pass
    human_judgment: false
  - id: D2
    description: "Authentic32Mode enum with Off, LegacyAccumulator, ProperSRC (default Off)"
    requirement: SRC-04
    verification:
      - kind: unit
        ref: "tests/SchroederTankCoreTest.cpp#Authentic32Mode exposes diagnostics-only enum values"
        status: pass
    human_judgment: false
  - id: D3
    description: "SchroederTankCore::reset() clears delay/LFO state without reallocation"
    requirement: TEST-10
    verification:
      - kind: unit
        ref: "tests/SchroederTankCoreTest.cpp#SchroederTankCore reset produces repeatable impulse response"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-07-08
status: complete
---

# Phase 13 Plan 01: r8brain Bootstrap + Core Reset Summary

**r8brain-free-src 7.1 CPM pin, Authentic32Mode diagnostics enum, and SchroederTankCore reset with impulse parity tests**

## Performance

- **Duration:** 12 min
- **Started:** 2026-07-08T20:11:00Z
- **Completed:** 2026-07-08T20:23:00Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Added `cmake/R8brain.cmake` with pinned `e71c31bf` commit and INTERFACE `r8brain` target
- Wired `include(R8brain)` and `target_link_libraries(SharedCode INTERFACE r8brain)` in root CMakeLists
- Created `Authentic32Mode` enum (Off / LegacyAccumulator / ProperSRC) for diagnostics-only SRC path selection
- Added `SchroederTankCore::reset()` clearing predelay, comb/APF, tank AP, and LFO phase
- TDD tests pass: impulse parity after reset and enum round-trip

## Task Commits

Each task was committed atomically:

1. **Task 1: r8brain CPM fetch and CMake INTERFACE target** - `ec30f3e` (feat) + cmake submodule `38bc0f5`
2. **Task 2 RED: failing reset/enum tests** - `e9211c3` (test)
3. **Task 2 GREEN: Authentic32Mode + reset()** - `b76d19b` (feat)

## Files Created/Modified
- `cmake/R8brain.cmake` - CPM fetch and INTERFACE include target for r8brain headers
- `CMakeLists.txt` - include R8brain module and link SharedCode to r8brain
- `source/Authentic32Mode.h` - diagnostics-only SRC mode enum (SRC-04)
- `source/SchroederTankCore.h` - public `reset()` for state clear without reallocation
- `tests/SchroederTankCoreTest.cpp` - reset impulse parity and Authentic32Mode tests

## Decisions Made
- Committed `R8brain.cmake` inside the `cmake` git submodule; parent commit updates submodule pointer
- `reset()` delegates to existing `DampedComb::reset()`, `SchroederAllpass::reset()`, and `predelayLine.reset()` — no `prepare()` or delay resize

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] cmake directory is a git submodule**
- **Found during:** Task 1 commit
- **Issue:** `git add cmake/R8brain.cmake` failed — cmake is a submodule
- **Fix:** Committed `R8brain.cmake` in cmake submodule (`38bc0f5`), then updated submodule pointer in parent repo
- **Files modified:** cmake/R8brain.cmake (submodule), CMakeLists.txt, cmake (submodule ref)
- **Verification:** CMake configure succeeds; CDSPResampler.h present
- **Committed in:** ec30f3e (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Submodule commit path is standard for this repo; no scope change.

## Issues Encountered
None beyond cmake submodule routing (resolved via deviation Rule 3).

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- r8brain headers available to SharedCode and Tests via INTERFACE link
- `SchroederTankCore::reset()` ready for `FixedRateAdapter` and `RateConverterPair` in 13-02/13-03
- `Authentic32Mode` enum ready for adapter mode branching in 13-03

## Self-Check: PASSED
- FOUND: cmake/R8brain.cmake
- FOUND: source/Authentic32Mode.h
- FOUND: source/SchroederTankCore.h (reset method)
- FOUND: ec30f3e, e9211c3, b76d19b

---
*Phase: 13-fixed-rate-adapter-r8brain*
*Completed: 2026-07-08*
