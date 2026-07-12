---
phase: 17-latency-pdc-adr-003
plan: 03
subsystem: testing
tags: [pdc, latency, adr-003, juce, catch2, policy-a]

requires:
  - phase: 17-01
    provides: SrcLatencyTable measured four-rate constants
  - phase: 17-02
    provides: FixedRateAdapter.getRoundTripLatencySamples delegate
provides:
  - Policy A setLatencySamples wiring in PluginProcessor
  - getSrcRoundTripLatencySamples delegates on tank and chain
  - LAT-03 mode-aware latency Catch2 gates
affects: [18-enablement, pluginval, adr-003]

tech-stack:
  added: []
  patterns:
    - "DSP adapter exposes latency count; processor reports via setLatencySamples"
    - "Target-path updateReportedLatency on authentic_color crossfade edge"

key-files:
  created: []
  modified:
    - source/SchroederTank32.h
    - source/GatedBloomChain.h
    - source/PluginProcessor.h
    - source/PluginProcessor.cpp
    - tests/LatencyTest.cpp

key-decisions:
  - "dynamic_cast to SchroederTank32 in GatedBloomChain; Fdn8Reverb test engines return 0"
  - "Synchronous setLatencySamples on prepare and toggle edge (no AsyncUpdater)"

patterns-established:
  - "updateReportedLatency(bool targetAuthenticOn) centralizes ADR-003 Policy A"
  - "RC1 zero latency when authentic_color off; measured SRC when on"

requirements-completed: [LAT-03]

coverage:
  - id: D1
    description: "RC1 default and authentic-off report zero latency samples"
    requirement: LAT-03
    verification:
      - kind: unit
        ref: "tests/LatencyTest.cpp#Plugin reports zero latency samples (RC1 default)"
        status: pass
      - kind: unit
        ref: "tests/LatencyTest.cpp#Plugin latency unchanged after prepare with authentic off"
        status: pass
    human_judgment: false
  - id: D2
    description: "authentic_color on reports SrcLatencyTable round-trip at 48 kHz"
    requirement: LAT-03
    verification:
      - kind: unit
        ref: "tests/LatencyTest.cpp#Plugin reports SRC latency when authentic_color on"
        status: pass
    human_judgment: false
  - id: D3
    description: "SRC latency tracks all four measured host rates with authentic on"
    requirement: LAT-03
    verification:
      - kind: unit
        ref: "tests/LatencyTest.cpp#Plugin SRC latency tracks host rate with authentic on"
        status: pass
    human_judgment: false
  - id: D4
    description: "Latency returns to zero when authentic_color toggled off after prepare"
    requirement: LAT-03
    verification:
      - kind: unit
        ref: "tests/LatencyTest.cpp#Plugin latency returns to zero when authentic off after prepare"
        status: pass
    human_judgment: false

duration: 20min
completed: 2026-07-09
status: complete
---

# Phase 17 Plan 03: Policy A PDC Wiring Summary

**ADR-003 Policy A host PDC: zero latency when 32k Color off, measured SRC round-trip when authentic on, with toggle-edge updates**

## Performance

- **Duration:** 20 min
- **Started:** 2026-07-08T22:26:00Z
- **Completed:** 2026-07-08T22:46:00Z
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments

- Added `getSrcRoundTripLatencySamples()` on SchroederTank32 and GatedBloomChain (dynamic_cast to tank)
- Implemented `updateReportedLatency(bool)` in PluginProcessor per ADR-003 Policy A
- Wired latency updates in `prepareToPlay` and on authentic_color crossfade edge in `processBlock`
- Expanded LatencyTest.cpp with `[LAT-03]` gates for RC1 zero path, four-rate SRC reporting, and off-after-on

## Task Commits

1. **Task 1: Latency query delegates** - `7e9efe4` (feat)
2. **Task 2: PluginProcessor Policy A wiring** - `02b027c` (test RED), `1f76484` (feat GREEN)
3. **Task 3: Mode-aware LatencyTest LAT-03 gates** - `02b027c` (test, delivered in TDD RED commit)

**Plan metadata:** skipped (commit_docs disabled)

## Files Created/Modified

- `source/SchroederTank32.h` - delegates to FixedRateAdapter round-trip count
- `source/GatedBloomChain.h` - tank dynamic_cast delegate, 0 for non-tank engines
- `source/PluginProcessor.h` - `updateReportedLatency` private helper
- `source/PluginProcessor.cpp` - Policy A setLatencySamples in prepare and toggle edge
- `tests/LatencyTest.cpp` - RC1 and LAT-03 mode-aware latency assertions

## Decisions Made

- Used dynamic_cast on reverb engine rather than extending IReverbEngine interface
- Synchronous setLatencySamples sufficient; no AsyncUpdater needed (tests pass)

## Deviations from Plan

None - plan executed exactly as written.

## TDD Gate Compliance

- RED commit `02b027c`: failing LAT-03 tests before implementation
- GREEN commit `1f76484`: Policy A wiring passes all LAT-03 and chain latency tests

## Issues Encountered

None

## User Setup Required

None

## Next Phase Readiness

- LAT-03 satisfied; Phase 18 enablement can proceed
- Full ctest green (192/192)

## Self-Check: PASSED

- FOUND: source/PluginProcessor.cpp
- FOUND: tests/LatencyTest.cpp
- FOUND: 7e9efe4
- FOUND: 02b027c
- FOUND: 1f76484

---
*Phase: 17-latency-pdc-adr-003*
*Completed: 2026-07-09*
